# OpenVanilla 行列輸入法 DIME 化・手把手重建手冊

> 適用情境：Mac 重置、換新電腦、或幫另一台機器建置。
> 預估時間：Xcode 下載及安裝 3~10 分鐘，實際動手操作約 15 分鐘。
> 最後更新：2026-07-04（v3 版）

---

## 這份手冊在建置什麼？

一個改造過的 OpenVanilla 行列輸入法，包含四項 DIME 化修改：

1. **空白鍵送出自訂詞**：先查自建詞庫、再查主碼表，自訂詞永遠排在候選列最前面。
2. **自訂詞優先、且不干擾候選字編號的選字邏輯**：自訂詞對應 `'` 選字鍵、不占用數字鍵，原碼表候選維持官方位置。例，自訂詞以 `xx` 組字時：`xx` ＋空白＋空白＝自訂詞；`xx` ＋ `1` ＝絲，數字鍵仍對應原候選字清單。
3. **可隨時編修的自建詞庫**：便利的熱重載機制。編輯詞庫後存檔，切換視窗或輸入法立即生效，不必重新編譯輸入法或新啟動系統。
4. **自訂詞突破四碼上限**：自訂詞字碼最長可到 32 鍵。

---

## 隨身包內容物

重建時需要以下檔案：

| 檔案 | 用途 |
|---|---|
| `ovim-array-dime-v3.patch` | 對 OpenVanilla 原始碼的修改（首選安裝方式） |
| `LegacyOVIMArray.cpp` | 修改後的完整原始碼（patch 套不上時的備案） |
| `LegacyOVIMArray.h` | 修改後的完整標頭檔（備案，與 .cpp 搭配使用） |
| `array-phrase.cin` | 詞庫**範本**（含格式示範，請填入自己的詞條；可以行列官方詞彙表為擴充基底：https://github.com/gontera/array30 ） |
| 本手冊 | 👀✨🧙‍♂️ |

---

## 第〇章：環境準備

### 0-1 安裝完整版 Xcode

至 **App Store** 搜尋「Xcode」（Apple 官方，榔頭圖示），下載安裝。體積超過 10 GB，請確認硬碟至少有 20 GB 可用空間。

裝完後**打開一次**，同意授權條款，跑完首次設定後即可關閉。

> 注意：`xcode-select --install` 裝的 command line tools **不夠用**，編輸入法需要完整版 Xcode。

### 0-2 讓終端機認得 Xcode

開啟「終端機」（`⌘ ＋ 空白鍵` 搜尋 Terminal），依序執行以下兩行。
（之後所有指令都是：複製 → 貼到終端機 → 按 Enter。）

```
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
```

> 會要求輸入 Mac 登入密碼。**輸入時螢幕完全不會顯示任何字元**，此為正常的安全設計，輸入完成後直接 Enter。

```
sudo xcodebuild -runFirstLaunch
```

> 這行會安裝 Xcode 的附加元件，畫面可能停一兩分鐘，不要中斷。
> 如果略過這步，之後編譯會噴一大串 `failed to load a required plug-in` 的錯誤──遇到了就回來補做這行。

### 0-3 驗證環境

```
xcodebuild -version
```

看到類似 `Xcode 26.x` 的版本號跳出來，環境就緒。

---

## 第一章：下載原始碼並套用修改

### 1-1 下載 OpenVanilla 原始碼

```
cd ~
git clone https://github.com/openvanilla/openvanilla.git
```

完成後家目錄會多出 `openvanilla` 資料夾。

### 1-2 套用 patch

先將隨身包裡的 `ovim-array-dime-v3.patch` 放到「下載項目」資料夾，然後：

```
cd ~/openvanilla
git apply ~/Downloads/ovim-array-dime-v3.patch
```

**沒有任何輸出、安靜跳回提示符號＝成功**（Unix 慣例：沒消息就是好消息）。

### 1-3 備案：patch 無法套用時

如果出現 `error: patch failed`（通常代表官方原始碼在打包後有更新、行號對不上了），改用完整檔案覆蓋。將隨身包裡的兩個原始碼檔放到下載項目，然後：

```
cp ~/Downloads/LegacyOVIMArray.cpp ~/openvanilla/Packages/OpenVanilla/Sources/OVIMArray/LegacyOVIMArray.cpp
cp ~/Downloads/LegacyOVIMArray.h ~/openvanilla/Packages/OpenVanilla/Sources/OVIMArray/include/LegacyOVIMArray.h
```

> 覆蓋法的風險：若官方日後大改架構，舊檔案可能與新框架不相容導致編譯失敗。若遇到問題，請把錯誤訊息帶去找 Claude。

---

## 第二章：放置自建詞庫

```
mkdir -p ~/Library/Application\ Support/OpenVanilla/UserData/Array
cp ~/Downloads/array-phrase.cin ~/Library/Application\ Support/OpenVanilla/UserData/Array/
```

> 指令中的 `\ `（反斜線＋空格）是告訴終端機「這個空格是資料夾名稱的一部分」，照抄即可。

驗證：

```
ls ~/Library/Application\ Support/OpenVanilla/UserData/Array/
```

看到 `array-phrase.cin` 就對了。這個位置就是日後熱重載讀取的自建詞庫本人。

---

## 第三章：編譯

```
cd ~/openvanilla
xcodebuild -project OpenVanilla.xcodeproj -scheme OpenVanilla -configuration Release
```

第一次編譯約 5~15 分鐘，畫面會滾大量文字。**黃色 `warning` 是正常的**，只要最後一行是：

```
** BUILD SUCCEEDED **
```

就成功了。若是 `** BUILD FAILED **`，一樣請將最後幾行紅色錯誤截圖去找 Claude。

---

## 第四章：安裝至系統

### 4-1 複製 App

```
mkdir -p ~/Library/Input\ Methods
cp -R ~/Library/Developer/Xcode/DerivedData/OpenVanilla-*/Build/Products/Release/OpenVanilla.app ~/Library/Input\ Methods/
```

> 路徑中的 `OpenVanilla-*` 星號是萬用字元──Xcode 會在每台電腦產生不同的雜湊碼資料夾（例如 `OpenVanilla-fmcaapcxljozcpcoyacuksxbbubu`），星號讓指令在任何機器上都找得到，不必手動查。

驗證：

```
ls ~/Library/Input\ Methods/
```

看到 `OpenVanilla.app` 即完成。

### 4-2 啟用輸入法

1. **重新開機**（讓系統重新掃描輸入法）。
2. 「系統設定 → 鍵盤 → 輸入方式」按「編輯…」或「＋」。
3. 找到「行列」（Array）加入。

---

## 第五章：驗收清單

切到行列輸入法。範本詞庫內建了兩條示範詞（`;;;` 測試詞、`opl` 範例詞），逐項測試：

| 輸入 | 預期結果 |
|---|---|
| `xx` ＋空白＋空白 | 測試詞（自訂詞，空白鍵直送） |
| `xx` ＋數字鍵 | 原候選字（數字鍵位置不受自訂詞影響） |
| `oh` ＋ `7` | 留（官方碼表第 7 位，前面有 ⎔ 佔位符） |
| 沒有自訂詞的任意字碼 | 候選順序與官方行列完全一致 |
| 編輯詞庫存檔 → 切換視窗 → 輸入新詞 | 新詞生效（熱重載） |

全過＝竣工。🎉

---

## 附錄 A：日常詞庫維護

**詞庫本尊位置**（Finder 按 `⌘ ＋ Shift ＋ G` 貼上可直達）：

```
~/Library/Application Support/OpenVanilla/UserData/Array/array-phrase.cin
```

**新增詞條**：在 `%chardef begin` 與 `%chardef end` 之間加一行，格式為「字碼、空格、詞」：

```
jcl 範例詞
```

存檔後切換一次視窗或輸入法即生效。

**注意事項**：

- 字碼只能用行列三十鍵：`a`–`z` 與 `;` `,` `.` `/`。**避免使用數字**（會與選字鍵打架）。
- 字碼上限 32 鍵。
- 同一組字盡量只對應一筆自訂詞（多掛時僅第一條能用空白鍵送出，其餘要靠 `字碼＋'＋數字`）。
- 檔案必須是 UTF-8 純文字。

**編輯器**：建議使用 CotEditor（App Store 免費）或 VS Code；macOS 內建的文字編輯程式 TextEdit 須先調成純文字模式並關閉智慧引號。

**改壞了怎麼辦**：格式錯誤時輸入法不會壞，只會安靜沿用上一版詞庫。發現「新詞沒生效」就是檢查檔案格式的訊號。

## 附錄 B：常見障礙排除

| 症狀 | 解法 |
|---|---|
| 點選開啟 `.cin` 跳「Apple 無法驗證」 | 從編輯器內部用「檔案 → 打開」開啟；或「系統設定 → 隱私權與安全性」往下滾動，會看到一行「已阻止使用 oooo.cin」附帶「強制打開」按鈕，點選後輸入本機密碼；或終端機執行 `xattr -d com.apple.quarantine 檔案路徑` 撕掉隔離標籤 |
| 編譯噴 `failed to load a required plug-in` | 執行 `sudo xcodebuild -runFirstLaunch` |
| 編譯前跳授權條款 | 同意後重下編譯指令 |
| 重開機後輸入法列表沒有行列 | 確認 4-1 的 `ls` 驗證有過；再登出登入一次 |
| 更新輸入法版本時 | 先切到英文輸入法 → `rm -rf ~/Library/Input\ Methods/OpenVanilla.app` → 重做 4-1 → 重開機 |

## 附錄 C：備份提醒

隨身包裡的 `array-phrase.cin` 是**打包當下的快照**。您的詞庫是活的、會持續更新，所以請養成習慣：

**定期將服役中的詞庫複製一份出來**（e.g. 備份至雲端硬碟），指令：

```
cp ~/Library/Application\ Support/OpenVanilla/UserData/Array/array-phrase.cin ~/Desktop/array-phrase-備份.cin
```

重建時改選用最新的備份取代隨身包裡的舊快照，讓順手的行列和你長長久久。

---

*本手冊由 Claude 整理撰寫。原始專案：OpenVanilla（https://github.com/openvanilla/openvanilla ，MIT 授權）；行列官方詞彙表：https://github.com/gontera/array30 。*
