# Budget Manager 
 
這是一個使用 GTK 開發的記帳軟體。 
 
## 安裝與執行 
**安裝 GTK：** 
```sh 
sudo apt-get install libgtk-3-dev 
``` 
 
**編譯與執行：** 
```sh 
gcc budget.c -o budget `pkg-config --cflags --libs gtk+-3.0` 
./budget 
``` 
