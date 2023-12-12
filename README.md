CM
==

A Remote Control Module, 8051 digital I/O module with Keil C

================

Controller Module Hardware & Protocol Document
===

* Writer: Kun Yi Chen
* Create Date: 2002/04/20
* Modification Date:
* 本文件中以 0x 為開頭的數字為代表 16 進制的值
* <> 為元素標記(element tag)

### Introduction:

   此文件僅供參考，並不能完全反應目前使用系統的情況
   這份文件是依據現存 CM5 的 SOURCE CODE，來進行逆向工程得到的。
   因此可能會發生『遺失舊有資訊，甚至有所錯誤』，這點需特別注意。

### Hardware:
Board Of Material (BOM)
---

* 27C512(program store)    -- Full 64K
* 61128 or 61256 (memory)  -- 16K or 32K, Address 0xB000(or 0x8000) ~ 0xFFFF
* GAL16v8(address decode)
* 93C46 (non-volatile memory) -- 128 Byte ( not use )
* DIP Switch (Port direction an CM ID) -- 4 ~ 7 with PID port direction, 0 ~ 3 with CM number of identification
* 8255 (programmable interface devce, PID)
  * address 0x2000 ( port A)
  * address 0x2001 ( port B)
  * address 0x2002 ( port C)
  * address 0x2003 ( control )

#### PID port directional

* the port A with bit7, if hot then it is input mode
* the port B with bit6, if hot then it is input mode
* the port CH with bit5, if hot then it is input mode
* the port CL with bit4, if hot then it is input mode

### Pin assignment:

#### DIP_SW(CM ID and PID port directional) -- use P1.0 ~ P1.3

#### MUXSEL(multiple select)                                 -- use P1.4

用來選擇DIP_SW 與 P1.0 ~ P1.3 對應
* 為 Hi 時 P1.0 ~ P1.3 對應到 DIP_SW 的 4 ~ 7 pin
* Low 時  P1.0 ~ P1.3 對應到 DIP_SW 的 0 ~ 3 pin

UART_DIR(RS485 Tx or Rx) -- use P3.4

* Hi 為 Tx, Low 為 Rx

#### WATCHDOG(reset watchdog timer) -- use P3.5

---------

CM(Controller Module) End Receive Protocol Packet defined/CM 的接收協定封包定義
===

#### 一個完整的 CM 能接受的控制封包，包含下列幾個元素，

依序為 **\<SOH>**, **\<NUM>**, **\<CMD>**, **\<VAL>**, **\<OPT>**, **\<CHK>** 代表的意義如下:

* \<SOH>  : 封包的開頭    (start of heading)
* \<NUM>  : CM 的編號     (number of CM)
* \<CMD>  : CM 的控制命令 (CM command)
* \<VAL>  : 參數          (parameter value)
* \<OPT>  : 選擇性參數    (optional parameter value)
* \<CHK>  : 封包檢查碼    (checksum)

所有元素的大小均為8個位元，即佔一個位元組的空間
以目前定義的封包大小來說均為7個位元組大

#### 下面說明如何設定或計算各元素的值

* \<SOH>  : 為 0x01 (always 1)
* \<NUM>  : 欲控制 CM 的編號,Master 編號為 0x00 (Master ID equal 0x00)
* \<CMD>  : 要求 CM 動作的命令
* \<VAL>  : \<CMD>的參數(必有)
* \<OPT>  : 選擇性參數，是依附在\<CMD>，有的 \<CMD> 命令並沒有
* \<CHK>  : 從\<NUM>開始到\<CHK>之前，將所有的值相互作 XOR 即得

#### 下面說明 \<CMD> 元素各個值所代表的意義

\<CMD> 目前定義有六種指令，分別用 **'D'**, **'R'**, **'W'**, **'T'**, **'P'**, **'S'** 六個大寫的 ASCII 字母表示之

* 'D'(0x44): 設定 PID(programmable interface device) 8255 的埠的輸出入
* 'R'(0x52): 讀取 PID 某埠的值
* 'W'(0x57): 寫一個值到PID某埠
* 'T'(0x54): 設定 Pulse 長度(以 10ms 為單位)
* 'P'(0x50): 產生一個 Pulse
* 'S'(0x53): 將 AIO Reset,透過 PID PortC7 的接腳產生一個 9HZ 的信號

##### 'D' Command 包含<VAL>與<OPT>兩參數

###### \<VAL>: 為 PID PORT 名稱，
   * 分別為 PORT A 的 'A'
   * PORT B 的 'B'
   * PORT C 高四位元(high nibble)部份的 'H'
   * 與 PORT C 低四位元(low nibble)部分的 'L'

###### \<OPT>: 代表輸出入(input/output)的方向，
   * 為代表輸入的'I'(input)與
   * 代表輸出的 'O' (output)

#### 'R' Command 只有\<VAL>參數

##### \<VAL> : 為 PID PORT 名稱

* 名稱同 'D' Command還增加了一個
* 各 PORT 目前方向狀態的 'd'

#### 'W' Command 包含\<VAL>與\<OPT>兩參數
##### \<VAL> : 為 PID PORT 名稱

 * 名稱同 'R' Command

##### \<OPT> : 要寫入的值

#### 'P' Command 包含\<VAL>與\<OPT>兩參數

##### \<VAL> : 為 PID PORT 名稱

 * 名稱同 'D' Command

##### \<OPT> : 產生 pulse PORT 的遮罩(mask)

#### 'T' Command 只有\<VAL>參數

##### \<VAL> : 為 PID PORT 名稱

* 名稱同 'D' Command

##### \<OPT> : Pulse 寬度的值 (unit of 10ms)

#### 'S' Command 只有\<VAL>參數

##### \<VAL> : 為 'N' 時，代表對 PID pin C.7 作一交替變化，頻率為 9 ~ 10 Hz (在 TRTS 裡，為 AIO WATCHDOG)

其他值代表停止此一信號變化

### Response

CM end reply packet defined/CM 回應封包定義

格式同接收封包具有 **\<SOH>**, **\<NUM>**, **\<CMD>**, **\<VAL>**, **\<CHK>** 等元素，少了一項 **\<OPT>**

下面說明如何設定或計算各元素的值
* \<SOH>  : 永遠是 1 (always 1)
* \<NUM>  : 控制程式編號 (always 0)
* \<CMD>  : 要求 CM 動作的命令
* \<VAL>  : <CMD>的參數(必有)
* \<CHK>  : 從<NUM>開始到<CHK>之前，將所有的值相互作 XOR 即得

下面說明 **\<CMD>** 元素各個值所代表的意義
<CMD>目前定義有四種指令，分別用 **'V'** , **'O'** , **'E'** , **'X'** 四個大寫的 ASCII 字母表示之

* 'V' (0x00): 傳回一個數值
* 'O' (0x00): 確認正確
* 'E' (0x00): 錯誤訊息(目前不被使用)
* 'X' (0x00): 傳回錯誤資訊

----
#### 'V' Command 的 \<VAL> 欄位意義

* \<VAL> : 要傳回的值

#### 'O' Command 的 \<VAL> 欄位意義

* \<VAL> : 固定為 'K'

#### 'E' Command 的 \<VAL> 欄位意義

* \<VAL> : 固定為 'R'

#### 'X' Command 的 \<VAL> 欄位意義

* \<VAL> : 目前有 9 個已經定義的值，分別如下
  * '1'(0x30) : 所接收的\<CMD>內容 CM 並未定義
  * '2'(0x31) : 跟在 **'D'** Command後，所接收的\<VAL>內容 CM 並未定義(即沒有這個 Port定義)
  * '3'(0x32) : PID Port 的方向設定，不能執行 **'R'** Command
  * '4'(0x33) : 跟在 **'R'** Command後，所接收的\<VAL>內容 CM 並未定義(即沒有這個 Port定義)
  * '5'(0x34) : PID Port 的方向設定，不能執行 **'W'** Command
  * '6'(0x35) : 跟在 **'W'** Command後，所接收的\<VAL>內容 CM 並未定義(即沒有這個 Port定義)
  * '7'(0x36) : 正在執行產生 Pulse 中，不接受新的產生 Pulse 命令
  * '8'(0x37) : 要執行產生 Pulse 的 PID Port 不是處於輸出的方向
  * '9'(0x38) : 跟在 **'P'** Command後，所接收的\<VAL>內容 CM 並未定義(即沒有這個 Port定義)
