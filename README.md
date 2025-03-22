# 進階記帳軟體

## 1. 專案介紹
本專案是一款基於 **GTK+** 開發的桌面應用程式，提供 **收入與支出管理、交易記錄、數據視覺化** 等功能，幫助使用者輕鬆管理個人財務。

## 2. 專案結構
```
budget_tracker/
├── budget.c              # 主程式，包含 UI 與邏輯
├── records.txt           # 儲存交易記錄的檔案
├── README.md             # 專案說明
```

## 3. 環境設定
### 1. 安裝必要套件（Linux）
```sh
sudo apt update
sudo apt install -y libgtk-3-dev build-essential
```

### 2. 編譯與執行
```sh
make
./budget_tracker
```

## 4. 主要功能
- **新增交易**：輸入 **類型（收入/支出）、描述、金額、日期**，新增記錄。
- **刪除交易**：選取交易後可刪除記錄。
- **編輯交易**：可修改已新增的交易。
- **儲存與讀取交易**：交易會自動儲存至 `records.txt`，並在開啟程式時自動載入。
- **圖表分析**：支援 **收入與支出的柱狀圖**，可視化財務狀況。

## 5. 操作介面
### 主要視窗
- **資產狀況**：顯示目前資金結餘。
- **交易記錄表格**：列出所有交易。
- **交易摘要區域**：顯示詳細記錄與計算結果。
- **圖表視覺化**：點擊按鈕可顯示 **收入/支出分析圖表**。

## 6. 檔案說明
- `records.txt`：存放所有交易記錄，格式為：
  ```
  類型 描述 金額 日期
  0 新資 5000 2025-03-01
  1 午餐 -100 2025-03-02
  ```
  `0` 代表收入，`1` 代表支出。



