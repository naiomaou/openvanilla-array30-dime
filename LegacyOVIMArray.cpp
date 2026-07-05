// OVIMArray.h: The Array Input Method
//
// Copyright (c) 2004-2008 The OpenVanilla Project (http://openvanilla.org)
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of OpenVanilla nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

//#define OV_DEBUG
#include "ctype.h"
#include "LegacyOVIMArray.h"
#include "OVLibrary.h"
#include "OVUtility.h"
#include <utility>
#include <cstdlib>
#include <sys/stat.h>
#ifdef OSX_INCLUDE
        #include <Carbon/Carbon.h>
#endif

using namespace std;
using namespace OV_Array;


void OVIMArrayContext::clear()
{
    // clear() is called at session start/stop (never mid-composition),
    // which makes it a safe point to hot-reload the user phrase table.
    parent->reloadUserPhraseTableIfNeeded();
    keyseq.clear();
    changeState(STATE_WAIT_KEY1);
}

bool OVIMArrayContext::isComposing()
{
    return state == STATE_WAIT_CANDIDATE;
}

void OVIMArrayContext::updateDisplay(OVBuffer* buf)
{
    buf->clear();
    if (keyseq.length()) {
        string str;
        keyseq.compose(str);
        buf->append(str.c_str());
    }
    buf->update();
}

int OVIMArrayContext::updateCandidate(OpenVanilla::OVCINDataTable *tab, OVBuffer *buf, OVCandidate *candibar)
{
    candidateStringVector.clear();
    auto pairs = tab->findChardefWithWildcard(string(keyseq.getSeq()));
    for (const auto& pair : pairs) {
        candidateStringVector.push_back(pair.second);
    }
    if (candidateStringVector.size() == 0) {
        clearCandidate(candibar);	
	}
    else {
        candi.prepare(&candidateStringVector,
                          const_cast<char*>(selKeys.c_str()), candibar);
	}
    return 1;
}

int OVIMArrayContext::WaitKey1(OVKeyCode* key, OVBuffer* buf, 
                               OVCandidate* candibar, OVService* srv)
{
    if (keyseq.length() != 1) {
        return 0;
    }

    if (!keyseq.hasWildcardCharacter()) {
        updateCandidateMerged(PHRASE_TAB, SHORT_TAB, buf, candibar);
    }

    char keycode = keyseq.getSeq()[0];

    if (keycode == 't') {
        buf->clear()->append((char*)"\xe7\x9a\x84")->update();
    }

    if (isprint(keycode) && keyseq.valid(keycode)) {
        changeState(STATE_WAIT_KEY2);
    }

    return 1;
}

int OVIMArrayContext::WaitKey2(OVKeyCode* key, OVBuffer* buf, 
                               OVCandidate* candibar, OVService* srv)
{
    if (keyseq.length() != 2) {
        return 0;
    }

    char keycode = keyseq.getSeq()[1];

    if (isWSeq(keyseq.getSeq()[0], keyseq.getSeq()[1])) {
        updateCandidate(tabs[MAIN_TAB], buf, candibar);
        buf->clear()->append(candidateStringVector[0].c_str())->update();
        changeState(STATE_WAIT_CANDIDATE);
    }
    else {
        if (!keyseq.hasWildcardCharacter()) {
            updateCandidateMerged(PHRASE_TAB, SHORT_TAB, buf, candibar);
        }

        if (isprint(keycode) && keyseq.valid(keycode)) {
            changeState(STATE_WAIT_KEY3);
        }
    }
    return 1;    
}

int OVIMArrayContext::WaitKey3(OVKeyCode* key, OVBuffer* buf, 
                               OVCandidate* candibar, OVService* srv)
{
    if (keyseq.length() >= 3) {
        if (!keyseq.hasWildcardCharacter()) {
            updateCandidateMerged(PHRASE_TAB, MAIN_TAB, buf, candibar);
        }
    }
    return 1;    
}


int OVIMArrayContext::WaitCandidate(OVKeyCode* key, OVBuffer* buf,
                                    OVCandidate* candibar, OVService* srv)
{
    const char keycode = key->code();

    if (keycode == ovkEsc || keycode == ovkBackspace) {
        clearAll(buf, candibar);
        changeState(STATE_WAIT_KEY1);
        return RET_DONE;
    }

    if (keycode == ovkDown || keycode == ovkRight ||
        (!candi.onePage() && keycode == ovkSpace)) {
        candi.pageDown()->update(candibar);
        return RET_DONE;
    }

    if (keycode == ovkUp || keycode == ovkLeft) {
        candi.pageUp()->update(candibar);
        return RET_DONE;
    }

    // enter == first candidate
    // space (when candidate list has only one page) == first candidate
    char c = key->code();
    // use the candidate list's *effective* selection keys, which may include
    // the ' label for user phrases in addition to the digit keys
    string effectiveSelKeys(candi.getSelKey());
    bool notSelkey = effectiveSelKeys.find(keycode) == string::npos;

    bool defaultSelKey = (keycode == ovkReturn || (candi.onePage() && keycode == ovkSpace));
    murmur("notSelkey: %d", notSelkey);
    if (defaultSelKey || notSelkey) 
        c = candi.getSelKey()[0];

    string output;

    if (candi.select(c, output)) {
        sendAndReset(output.c_str(), buf, candibar, srv);
        if (notSelkey && !defaultSelKey) {
            return RET_CONTINUE;
        }
        return RET_DONE;
    }
    
    return RET_PASS;
}


void OVIMArrayContext::queryKeyName(const char *keys, std::string& outKeyNames)
{
    int len = (int)strlen(keys);
    for (int i = 0; i < len; i++) {
        string inKey(keys, i, 1);
        outKeyNames.append(tabs[MAIN_TAB]->findKeyname(inKey));
    }
}

void OVIMArrayContext::sendAndReset(const char *ch, OVBuffer* buf, 
                                    OVCandidate* candibar, OVService* srv)
{
    // If ⎔, prompt and reset state.
    if (!strcmp(ch, "⎔")) {
        clearAll(buf, candibar);
        changeState(STATE_WAIT_KEY1);
        srv->notify("無此字");
        return;
    }

    bool notifySP = false;
    string notifyText;

    // lookup special code
    if (parent->isForceSP() || parent->isAutoSP()) {

        // Special table is short enough to allow sequential search.
        OpenVanilla::OVFastKeyValuePairMap* kvm = tabs[SPECIAL_TAB]->chardefMap();
        string searchValue(ch);
        string matchKey;
        for (size_t i = 0, len = kvm->size(); i < len; i++) {
            const auto& kv = kvm->keyValuePairAtIndex(i);
            if (searchValue == kv.second) {
                matchKey = kv.first;
                break;
            }
        }

        if (!matchKey.empty()) {
            int splen = (int)matchKey.length();
            const char *spcode = matchKey.c_str();
            if (!equal(spcode, spcode+splen, keyseq.getSeq())) {
                stringstream s;
                string keynames;
                queryKeyName(spcode, keynames);
                s << ch << ": " << keynames;
                notifyText = s.str();
                notifySP = true;
            }
        }
    }

    if (!(parent->isForceSP() && notifySP)) {
        buf->clear()->append(ch)->send();
    }
    else {
        buf->clear()->update();
        srv->beep();
    }

    if (notifySP) {
        srv->notify(notifyText.c_str());
    }

    clearCandidate(candibar);
    keyseq.clear();
    changeState(STATE_WAIT_KEY1);
}

void OVIMArrayContext::clearAll(OVBuffer* buf, OVCandidate* candi_bar)
{
    clearCandidate(candi_bar);
    buf->clear()->update();
    keyseq.clear();
}

void OVIMArrayContext::clearCandidate(OVCandidate *candi_bar)
{
    candi.cancel();
    candi_bar->hide()->clear();
}   

int OVIMArrayContext::selectCandidate(int num, string& out)
{
	char c = candi.getSelKey()[num];
	return candi.select(c, out);
}

int OVIMArrayContext::keyEvent(OVKeyCode* key, OVBuffer* buf, 
                               OVCandidate* candi_bar, OVService* srv)
{
    int ret = 0;
    const char keycode = key->code();
    const bool validkey = keyseq.valid(keycode) || 
      ( keyseq.getSeq()[0] == 'w' && isdigit(keycode) );

    murmur("OVIMArray state: %d", state);
    if (!keyseq.length() && !isprint(keycode)) {
        return 0;
    }
    if (!keyseq.length() && key->isFunctionKey()) {
        return 0;
    }

    if (isprint(key->code()) && key->isCapslock() && keyseq.length() == 0) {
        char cbuf[2];
            if (key->isShift()) {
                snprintf(cbuf, sizeof(cbuf), "%c", toupper(key->code()));
            }
            else {
                snprintf(cbuf, sizeof(cbuf), "%c", tolower(key->code()));
            }
        buf->append(cbuf)->send();
        return 1;
    }

    if (key->isShift() && isprint(keycode) && keyseq.length() == 0 && keycode != '*' && keycode != '?') {
        char k[2] = { (char)tolower(keycode), 0 };
        buf->append(k)->send();
        return 1;
    }

    if (!keyseq.length() && !validkey) {
        char k[2] = { (char)tolower(keycode), 0 };
        buf->append(k)->send();
        return 1;
    }

    if (keycode == ovkEsc){
        clearAll(buf, candi_bar);
        changeState(STATE_WAIT_KEY1);
        return 1;
    }

	// cancels candidate window if wildcard character is entered
	if (!keyseq.hasWildcardCharacter() && (keycode == '?' || keycode== '*')) {
		murmur("candidate canceled because of wildcard");
		clearCandidate(candi_bar);
		changeState(STATE_WAIT_KEY3);
	}

    if( state == STATE_WAIT_CANDIDATE) {
        int r = WaitCandidate(key, buf, candi_bar, srv);
        if (r != RET_CONTINUE) {
            return r;
        }
    }

    if (candi.onDuty() && isdigit(keycode) && 
        !(keyseq.length() == 1 && isWSeq(keyseq.getSeq()[0],keycode))) {
        string c;
        if (candi.select(keycode, c)){
            if (c != "?" ) {
                sendAndReset(c.c_str(), buf, candi_bar, srv);
            }
            else {
                srv->beep();
                if (state <= STATE_WAIT_KEY3) { //dirty hack to set duty=1
                    updateCandidateMerged(PHRASE_TAB, SHORT_TAB, buf, candi_bar);
                }
            }
        }
        return 1;
    }

    if (keyseq.length() && keycode == '\'') {
        commitKeySeq(PHRASE_TAB, "無此詞彙", buf, candi_bar, srv);
        return 1;
    }

    if (keyseq.length() && keycode == ovkSpace) {
        // Detect if it is ",,sp" to toggle SP mode.
        if (isForceSPSeq()) {
            bool newState = !parent->isForceSP();
            stringstream s;
            s << "快打模式：" << (newState ? "啟用" : "關閉");
            parent->setForceSP(newState);
            srv->notify(s.str().c_str());
            clearAll(buf, candi_bar);
            changeState(STATE_WAIT_KEY1);
            return 1;
        }
        // DIME-style: user phrases take precedence over the main table
        commitKeySeqMerged(PHRASE_TAB, MAIN_TAB, "無此字", buf, candi_bar, srv);
        return 1;
    }

    if (isprint(keycode) && validkey) {
        if (keyseq.length() >= 5 ||
           (keyseq.length() == 4 && keycode != 'i')) {
            // allow sequences longer than the Array30 limit as long as
            // they can still lead to a user phrase entry
            if (!phrasePrefixExists(keycode)) {
                return 1;
            }
        }
        murmur("keyseq add [%c]", keycode);
        keyseq.add(keycode);
        updateDisplay(buf);
        ret = 1;
    }
    else if (key->code() == ovkDelete || key->code() == ovkBackspace) {
        keyseq.remove();
        updateDisplay(buf);
        if(keyseq.length() == 0 && candi.onDuty())
            clearCandidate(candi_bar);
        changeBackState(state);
        ret = 1;
    } else {
        srv->beep();
        updateDisplay(buf);
        return 1;
    }
    dispatchStateHandler(key, buf, candi_bar, srv);
    return ret;
}

void OVIMArrayContext::commitKeySeq(int table, const char* errorMessage, OVBuffer* buf,
                                    OVCandidate* candi_bar, OVService* srv) {

    if (keyseq.hasOnlyWildcardCharacter()) {
        clearAll(buf, candi_bar);
        changeState(STATE_WAIT_KEY1);
        return;
    }

    auto pairs = tabs[table]->findChardefWithWildcard(OpenVanilla::OVWildcard(keyseq.getSeq()));
    candidateStringVector.clear();
    for (const auto& pair : pairs) {
        candidateStringVector.push_back(pair.second);
    }
    string c;
    if (candidateStringVector.size() == 1) {
        if (selectCandidate(0, c)) {
            sendAndReset(c.c_str(), buf, candi_bar, srv);
        }
    }
    else if (candidateStringVector.size() > 1) {
        updateCandidate(tabs[table], buf, candi_bar);
        if (selectCandidate(0, c)) {
            buf->clear()->append(c.c_str())->update();
        }
        changeState(STATE_WAIT_CANDIDATE);
    } else {
        clearAll(buf, candi_bar);
        changeState(STATE_WAIT_KEY1);
        srv->notify(errorMessage);
    }
}

// Merged candidate lookup: primary table (user phrases) results come first,
// then the secondary table, with duplicates removed. This gives DIME-style
// priority to user-defined phrases.
int OVIMArrayContext::updateCandidateMerged(int primaryTable, int secondaryTable,
                                            OVBuffer *buf, OVCandidate *candibar)
{
    candidateStringVector.clear();
    string seq(keyseq.getSeq());

    if (tabs[primaryTable]) {
        auto primaryPairs = tabs[primaryTable]->findChardefWithWildcard(seq);
        for (const auto& pair : primaryPairs) {
            candidateStringVector.push_back(pair.second);
        }
    }

    size_t phraseCount = candidateStringVector.size();

    if (tabs[secondaryTable]) {
        auto secondaryPairs = tabs[secondaryTable]->findChardefWithWildcard(seq);
        for (const auto& pair : secondaryPairs) {
            // NOTE: no deduplication here. Array tables use the placeholder
            // character (⎔) to pin candidates to fixed positions so that
            // digit-key selection matches the official Array30 layout.
            // Removing "duplicates" would collapse those placeholders and
            // shift every candidate's position.
            candidateStringVector.push_back(pair.second);
        }
    }

    if (candidateStringVector.size() == 0) {
        clearCandidate(candibar);
    }
    else {
        // DIME-style numbering: user phrases are shown first but labeled
        // with ' instead of consuming the digit selection keys, so the
        // original table candidates keep their official Array30 positions
        // (pressing 7 still selects the table's 7th candidate). Space
        // selects the first candidate, i.e. the user phrase when present.
        string effectiveSelKeys(selKeys);
        size_t room = 31 - selKeys.length(); // OVCandidateList selkey buffer is char[32]
        if (phraseCount > 0 && phraseCount <= room) {
            effectiveSelKeys = string(phraseCount, '\'') + selKeys;
        }
        candi.prepare(&candidateStringVector,
                          const_cast<char*>(effectiveSelKeys.c_str()), candibar);
    }
    return 1;
}

void OVIMArrayContext::commitKeySeqMerged(int primaryTable, int secondaryTable,
                                          const char* errorMessage, OVBuffer* buf,
                                          OVCandidate* candi_bar, OVService* srv)
{
    if (keyseq.hasOnlyWildcardCharacter()) {
        clearAll(buf, candi_bar);
        changeState(STATE_WAIT_KEY1);
        return;
    }

    updateCandidateMerged(primaryTable, secondaryTable, buf, candi_bar);

    string c;
    if (candidateStringVector.size() == 1) {
        if (selectCandidate(0, c)) {
            sendAndReset(c.c_str(), buf, candi_bar, srv);
        }
    }
    else if (candidateStringVector.size() > 1) {
        if (selectCandidate(0, c)) {
            buf->clear()->append(c.c_str())->update();
        }
        changeState(STATE_WAIT_CANDIDATE);
    } else {
        clearAll(buf, candi_bar);
        changeState(STATE_WAIT_KEY1);
        srv->notify(errorMessage);
    }
}

// Returns true if the current key sequence extended by nextKey is still a
// prefix of at least one user phrase entry. Used to lift the 4-key limit
// for user phrases with longer codes.
bool OVIMArrayContext::phrasePrefixExists(char nextKey)
{
    if (!tabs[PHRASE_TAB]) {
        return false;
    }
    if (keyseq.hasWildcardCharacter()) {
        return false;
    }

    string prefix(keyseq.getSeq());
    prefix.push_back((char)tolower(nextKey));

    OpenVanilla::OVFastKeyValuePairMap* kvm = tabs[PHRASE_TAB]->chardefMap();
    if (!kvm) {
        return false;
    }

    for (size_t i = 0, len = kvm->size(); i < len; i++) {
        const auto& kv = kvm->keyValuePairAtIndex(i);
        if (kv.first.length() >= prefix.length() &&
            kv.first.compare(0, prefix.length(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

void OVIMArrayContext::dispatchStateHandler(OVKeyCode* key, OVBuffer* buf, 
                                            OVCandidate* candi_bar, OVService* srv)
{
    switch(state){
        case STATE_WAIT_KEY1:
            WaitKey1(key, buf, candi_bar, srv);
            break;
        case STATE_WAIT_KEY2:
            WaitKey2(key, buf, candi_bar, srv);
            break;
        case STATE_WAIT_KEY3:
            WaitKey3(key, buf, candi_bar, srv);
            break;
        default:
            break;
    }
}

void OVIMArrayContext::changeBackState(STATE s)
{
    switch(s){
        case STATE_WAIT_CANDIDATE:
        case STATE_WAIT_KEY2: 
            changeState(STATE_WAIT_KEY1); 
            break;
        case STATE_WAIT_KEY3: 
            if (keyseq.length() == 2)
                changeState(STATE_WAIT_KEY2); 
            else if (keyseq.length() == 1)
                changeState(STATE_WAIT_KEY1);
            break;
        default: break;
    }
}

void OVIMArrayContext::changeState(STATE s)
{
    murmur("OVIMArray change state: %d -> %d", state, s);
    state = s;
}

int OVIMArray::initialize(OVDictionary *conf, OVService* s, const char *path)
{
    char arraypath[PATH_MAX];
    char buf[PATH_MAX];
    char *cinfiles[] = {
        (char *)"%sarray30.cin",
        (char *)"%sarray-shortcode.cin",
        (char *)"%sarray-special.cin",
        (char *)"%sarray-phrase.cin",
    };

#ifdef OSX_INCLUDE
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("org.openvanilla.ModulePackage.OVIMArray"));
    if (!bundle)
		return 0;

    CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
    if (!url) 
		return 0;

    CFURLGetFileSystemRepresentation(url, TRUE, (UInt8*)buf, sizeof(buf) - 1);
    snprintf(arraypath, sizeof(arraypath), "%s/", buf);
#else
    // snprintf(arraypath, sizeof(arraypath), "%sOVIMArray%s", path, s->pathSeparator());
    snprintf(arraypath, sizeof(arraypath), "%sArray%s", path, s->pathSeparator());
#endif
    printf("OVIMArray: data dir %s", arraypath);

    for (int i = 0; i < 4 ; i++) {
        OpenVanilla::OVCINDataTableParser parser;
        snprintf(buf, sizeof(buf), cinfiles[i], arraypath);
        murmur("OVIMArray: open cin %s", buf);
        tabs[i] = parser.CINDataTableFromFileName(buf);
    }

    // DIME-style user phrase support: if the user maintains their own
    // array-phrase.cin in Application Support, it overrides the bundled one
    // and is hot-reloaded whenever its modification time changes.
    userPhrasePath[0] = 0;
    userPhraseMTime = 0;
    const char* home = getenv("HOME");
    if (home) {
        snprintf(userPhrasePath, sizeof(userPhrasePath),
                 "%s/Library/Application Support/OpenVanilla/UserData/Array/array-phrase.cin",
                 home);
        reloadUserPhraseTableIfNeeded();
    }

    updateConfig(conf);
    return 1;
}

void OVIMArray::reloadUserPhraseTableIfNeeded()
{
    if (!userPhrasePath[0]) {
        return;
    }

    struct stat st;
    if (stat(userPhrasePath, &st) != 0) {
        // user file absent: keep whatever table is currently loaded
        return;
    }

    if ((long)st.st_mtime == userPhraseMTime) {
        return;
    }

    OpenVanilla::OVCINDataTableParser parser;
    OpenVanilla::OVCINDataTable* newTable =
        parser.CINDataTableFromFileName(string(userPhrasePath));
    if (!newTable) {
        // parse failure: keep the old table so typing keeps working
        return;
    }

    OpenVanilla::OVCINDataTable* oldTable = tabs[OV_Array::PHRASE_TAB];
    tabs[OV_Array::PHRASE_TAB] = newTable;
    userPhraseMTime = (long)st.st_mtime;

    if (oldTable) {
        delete oldTable;
    }
}

int OVIMArray::updateConfig(OVDictionary *conf){
    const char *AutoSP = "\xE7\x89\xB9\xE5\x88\xA5\xE7\xA2\xBC\xE6\x8F\x90\xE7\xA4\xBA";
    const char *ForceSP = "\xE5\xBF\xAB\xE6\x89\x93\xE6\xA8\xA1\xE5\xBC\x8F";

    if (!conf->keyExist(AutoSP)) {
        conf->setInteger(AutoSP, 1);
    }

    if (!conf->keyExist(ForceSP)) {
        conf->setInteger(ForceSP, 0);
    }

    cfgAutoSP = conf->getInteger(AutoSP);
    cfgForceSP = conf->getInteger(ForceSP);

    return 1;
}

OV_SINGLE_MODULE_WRAPPER(OVIMArray);

