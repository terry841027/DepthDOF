# ZDepthDOF — 建置說明

## 只需要一個工具（免費）

**Visual Studio 2022 Community**
https://visualstudio.microsoft.com/vs/community/

安裝時勾選：**「使用 C++ 的桌面開發」**
（會自動包含 MSVC 編譯器 + CMake + Windows SDK）

不需要 Git，不需要 GitHub，不需要任何其他東西。

---

## 建置步驟

從開始選單搜尋並開啟：
**「Developer Command Prompt for VS 2022」**

```bat
cd C:\Users\terry\Documents\ZDepthDOF
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

完成後 DLL 自動複製到：
`C:\Users\terry\Documents\Resolume Arena\Extra Effects\ZDepthDOF.dll`

---

## 在 Arena 中使用

1. 重新開啟 Resolume Arena
2. Effects 面板搜尋 **ZDepthDOF**
3. 將 iPhone HX Camera 載入 clip，加上此效果

---

## 參數

| 參數 | 說明 |
|---|---|
| Focal Plane Y | 對焦水平線（0=底部/近景，1=頂部/遠景）|
| Sharp Zone | 清晰帶寬度，越小景深越淺 |
| Max Blur | 最大模糊半徑 |
| Luma to Depth | 亮度對深度估算的混合比例 |
| Near Boost | 近景額外模糊倍率 |
| Quality | 採樣品質（8–32 samples）|
| Show Depth Map | 顯示深度灰階圖，方便調參 |
