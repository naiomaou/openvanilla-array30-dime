# 於 macOS 重現 DIME 手感的行列 30 輸入法

一份針對 [OpenVanilla](https://github.com/openvanilla/openvanilla) 行列模組的小修改，讓 Windows [DIME](https://github.com/jrywu/DIME) 行列輸入法使用者，可以更順利地將原使用經驗還原至 macOS 環境，降低轉換系統所面臨的輸入法摩擦力。

※ 寫在前面： 本專案所修改之程式碼、patch 與建置手冊由 [Claude](https://claude.ai/new)（Anthropic）協作完成。

## 這份修改做了什麼？

1. **透過空白鍵送出自訂詞**：保留 `'` 鍵送出自訂詞方案，加入空白鍵直接推送的方式，還原 DIME 輸入手感。
2. **自訂詞優先、且不干擾候選字編號的選字邏輯**：自訂詞對應 `'` 候選清單、不占用數字鍵；原候選字順序維持既定位置。
3. **可隨時編修的自建詞庫**：便利的熱重載機制。詞庫存放於 `~/Library/Application Support/OpenVanilla/UserData/Array/array-phrase.cin`，編輯存檔後切換視窗或輸入法立即生效，不必重新編譯輸入法或重新啟動系統。
4. **突破四碼上限**：自訂詞字碼最長可達 32 鍵。

## 內容物

| 檔案 | 用途 |
|---|---|
| `OpenVanilla-DIME-建置手冊.md` | **從零開始的手把手建置教學**，包含環境安裝、編譯、疑難排解等 |
| `ovim-array-dime-v3.patch` | 對 OpenVanilla 原始碼的修改（首選安裝方式） |
| `LegacyOVIMArray.cpp` / `LegacyOVIMArray.h` | 修改後的完整原始碼（patch 無法安裝時的備案） |
| `array-phrase.cin` | 自建詞庫範本（含格式示範） |

## 快速開始

請直接閱讀 [建置手冊](OpenVanilla-DIME-建置手冊.md)，從安裝 Xcode 到驗收測試，預估動手時間約十五分鐘（不含 Xcode 下載）。

**系統需求**：macOS、完整版 Xcode（App Store 免費下載，約 10 GB）。

## 致謝

- [OpenVanilla](https://github.com/openvanilla/openvanilla) ── 本修改的基礎，感謝 The OpenVanilla Project 全體貢獻者二十餘年的維護（MIT 授權，修改後的原始碼檔案均保留原版權聲明）
- [DIME](https://github.com/jrywu/DIME) ── 自建詞庫行為的設計藍本
- [gontera/array30](https://github.com/gontera/array30) ── 行列 30 官方碼表與詞彙表的整理與維護

## 授權與免責聲明

本 repo 中修改以 MIT 授權釋出。OpenVanilla 原始碼之版權屬 The OpenVanilla Project 及其貢獻者所有，授權條款詳見各檔案檔頭、本 repo 內的 [LICENSE-OpenVanilla.txt](LICENSE-OpenVanilla.txt)（原專案授權全文），以及原專案 LICENSE。

此為個人使用需求所做的修改，依現狀提供、不附任何保證；輸入法屬於高頻率使用的系統元件，建置前請詳讀手冊，並自行承擔使用風險。
