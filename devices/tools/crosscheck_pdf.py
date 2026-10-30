#!/usr/bin/env python3
"""코드 헤더에 적은 레지스터 오프셋·비트 이름을 AMD PG 문서 텍스트(pdftotext -layout 결과)와 자동 대조한다.
사용: crosscheck_pdf.py <폴더>  (폴더에 pg079.txt pg090.txt pg096.txt pg142.txt pg153.txt, 선택으로 mcp2515.txt, nrf24.txt). PDF 는 저장소에 넣지 않는다(재배포 문제).
W5500·MAX7219·AXI GPIO·HD44780·아날로그 센서 블록은 PDF 텍스트가 아니라 저장소 안의 */ref/*_facts.txt(데이터시트 본문 요약 등에서 옮긴 사실표)를 쓴다.
--facts-only 를 주면 PDF 텍스트 블록은 건너뛰고 사실표 블록만 검사한다(PDF 텍스트 없이 실행 가능).
한계: 이름과 숫자가 같은 줄/근처에 있는지만 본다. 비트 의미의 세부는 사람이 읽어 확인한 것이다."""
import re,sys,os
FACTS_ONLY = '--facts-only' in sys.argv     # PDF 텍스트 없이 사실표 블록만 (PDF 텍스트 블록은 건너뛰고 그 사실을 출력)
def load(p):
    if FACTS_ONLY and not os.path.exists(p): return []
    return open(p,errors='ignore').read().splitlines()
def has(lines, num, kws, window=0):
    """num 이 들어간 줄에서 kws(모두) 가 같은 줄 또는 window 줄 이내에 있는지"""
    for i,l in enumerate(lines):
        if re.search(num,l):
            blob=' '.join(lines[max(0,i-window):i+window+1]).lower()
            if all(k.lower() in blob for k in kws): return True
    return False
res=[]
def chk(doc,lines,label,num,kws,w=0):
    ok=has(lines,num,kws,w); res.append((doc,label,ok))
_a=[a for a in sys.argv[1:] if not a.startswith('--')]
D=_a[0] if _a else '.'
P=os.path.join(D,'pg153.txt'); L=load(P)
for lab,num,k in [('SRR 0x40',r'\b40h?\b|0x40',['reset']),('SPICR 0x60',r'\b60h?\b|0x60',['control']),('SPISR 0x64',r'64h?\b|0x64',['status']),
  ('DTR 0x68',r'68h?\b|0x68',['transmit']),('DRR 0x6C',r'6Ch?\b|0x6C',['receive']),('SPISSR 0x70',r'70h?\b|0x70',['slave select']),
  ('DGIER 0x1C',r'1Ch?\b|0x1C',['global']),('IPISR 0x20',r'\b20h?\b|0x20',['interrupt status']),('IPIER 0x28',r'28h?\b|0x28',['interrupt enable'])]:
    chk('PG153',L,lab,num,k,2)
for lab,num,k in [('CR bit9 LSB',r'^\s*9\s',['lsb first']),
  ('CR bit6 Rx reset',r'^\s*6\s',['rx fifo reset']),('CR bit5 Tx reset',r'^\s*5\s',['tx fifo reset']),('CR bit4 CPHA',r'^\s*4\s',['cpha']),
  ('CR bit3 CPOL',r'^\s*3\s',['cpol']),('CR bit2 master',r'^\s*2\s',['master']),('CR bit1 SPE',r'^\s*1\s',['spe']),('CR bit0 loop',r'^\s*0\s',['loop']),
  ('SR bit0 Rx_Empty',r'^\s*0\s',['rx_empty']),('SR bit2 Tx_Empty',r'^\s*2\s',['tx_empty']),('SR bit3 Tx_Full',r'^\s*3\s',['tx_full'])]:
    chk('PG153',L,lab,num,k,0)
# 이름이 여러 줄에 걸친 칸은 창 2줄(이웃 비트 이름이 겹치지 않는 키워드를 사용)
chk('PG153',L,'CR bit8 inhibit',r'^\s*8\s',['inhibit'],2)
chk('PG153',L,'CR bit7 MSS',r'^\s*7\s',['manual slave'],2)
chk('PG153',L,'SRR 값 0xA',r'0x0000_?000a|0x0000000a',['reset'],3)
L=load(os.path.join(D,'pg096.txt'))
for lab,num,k in [('SRR 0x000',r'0x000',['srr']),('MSR 0x004',r'0x004',['msr']),('BRPR 0x008',r'0x008',['brpr']),('BTR 0x00C',r'0x00C',['btr']),
  ('SR 0x018',r'0x018',['sr']),('ISR 0x01C',r'0x01C',['isr']),('IER 0x020',r'0x020',['ier']),('ICR 0x024',r'0x024',['icr']),
  ('TXFIFO 0x030',r'0x030',['txfifo']),('TXHPB 0x040',r'0x040',['txhpb']),('RXFIFO 0x050',r'0x050',['rxfifo'])]:
    chk('PG096',L,lab,num,k)
for lab,num,k in [('SRR CEN bit1',r'^\s*1\s',['cen']),('SRR SRST bit0',r'^\s*0\s',['srst']),('MSR LBACK bit1',r'^\s*1\s',['lback']),
  ('SR TXFLL bit10',r'^\s*10\s',['txfll']),('SR TXBFLL bit9',r'^\s*9\s',['txbfll']),('ISR RXNEMP bit7',r'^\s*7\s',['rxnemp']),('ISR RXOK bit4',r'^\s*4\s',['rxok']),('ISR ERROR bit8',r'^\s*8\s',['error']),
  ('BTR SJW 8:7',r'8:7',['sjw']),('BTR TS2 6:4',r'6:4',['ts2']),('BTR TS1 3:0',r'3:0',['ts1'])]:
    chk('PG096',L,lab,num,k,1)
L=load(os.path.join(D,'pg142.txt'))
for lab,num,k in [('Rx 0x00',r'^\s*0h\s',['rx fifo']),('Tx 0x04',r'0x04|\b04h',['tx fifo']),('STAT 0x08',r'0x08|\b08h',['status']),('CTRL 0x0C',r'0x0C|\b0Ch',['control'])]:
    chk('PG142',L,lab,num,k,1)
for lab,num,k in [('STAT bit0 valid',r'^\s*(7|6|5|4|3|2|1|0)\s',[])]: pass
for lab,kw in [('Parity Error',['parity error']),('Frame Error',['frame error']),('Overrun',['overrun error']),('Intr Enabled',['intr enabled']),('Tx FIFO Full',['tx fifo full']),('Tx FIFO Empty',['tx fifo empty']),('Rx FIFO Full',['rx fifo full']),('Rx FIFO Valid Data',['rx fifo valid data'])]:
    chk('PG142',L,lab+' 존재','.',kw)
L=load(os.path.join(D,'pg090.txt'))
for lab,num,k in [('GIE 0x01C',r'01C',['gie']),('ISR 020h',r'020h',['isr']),('IER 0x028',r'028',['ier']),('SOFTR 0x040',r'040',['softr']),('CR 100h',r'100h',['cr']),('SR 0x104',r'104',['sr']),
  ('TX_FIFO 0x108',r'108',['tx_fifo']),('RX_FIFO 0x10C',r'10C',['rx_fifo']),('ADR 0x110',r'110',['adr']),('TX_FIFO_OCY 0x114',r'114',['tx_fifo_ocy']),('RX_FIFO_OCY 0x118',r'118',['rx_fifo_ocy']),('RX_FIFO_PIRQ 0x120',r'120',['rx_fifo_pirq'])]:
    chk('PG090',L,lab,num,k,1)
L=load(os.path.join(D,'pg079.txt'))
for lab,num,k in [('TCSR0 0h',r'\b0h\b',['tcsr0']),('TLR0 04h',r'04h',['tlr0']),('TCR0 08h',r'08h',['tcr0']),('TCSR1 10h',r'\b10h\b',['tcsr1']),('TLR1 14h',r'14h',['tlr1']),('TCR1 18h',r'18h',['tcr1'])]:
    chk('PG079',L,lab,num,k)
for lab,num,k in [('MDT bit0',r'^\s*0\s',['mdt0']),('UDT bit1',r'^\s*1\s',['udt0']),('GENT bit2',r'^\s*2\s',['gent0']),('CAPT bit3',r'^\s*3\s',['capt0']),('ARHT bit4',r'^\s*4\s',['arht0']),('LOAD bit5',r'^\s*5\s',['load0']),('ENIT bit6',r'^\s*6\s',['enit0']),('ENT bit7',r'^\s*7\s',['ent0']),('TINT bit8',r'^\s*8\s',['t0int']),('PWMA bit9',r'^\s*9\s',['pwma0']),('ENALL bit10',r'^\s*10\s',['enall'])]:
    chk('PG079',L,lab,num,k)

# ---- MCP2515 (선택: mcp2515.txt = DS20001801K 의 pdftotext -layout 결과). Table 11-2(레지스터 요약), Table 12-1(SPI 명령), 레지스터 설명 ----
MP=os.path.join(D,'mcp2515.txt')
if os.path.exists(MP):
    L=load(MP)
    def row(name,addr):
        """Table 11-2 의 한 행에서 주소 뒤 비트 이름 8개(bit7→bit0)를 돌려준다"""
        for l in L:
            m=re.match(r'\s*'+name+r'\s+'+addr+r'\s+(.*)$',l)
            if m:
                tok=m.group(1).split()
                return tok[:8]
        return None
    def bits(name,addr,expect):
        r=row(name,addr); ok=bool(r) and all(r[7-b].upper()==n.upper() for b,n in expect.items())
        res.append(('MCP2515',f'{name} {addr}h 비트 '+','.join(f'{b}={n}' for b,n in expect.items()),ok))
    bits('CANINTE','2B',{7:'MERRE',6:'WAKIE',5:'ERRIE',4:'TX2IE',3:'TX1IE',2:'TX0IE',1:'RX1IE',0:'RX0IE'})
    bits('CANINTF','2C',{7:'MERRF',6:'WAKIF',5:'ERRIF',4:'TX2IF',3:'TX1IF',2:'TX0IF',1:'RX1IF',0:'RX0IF'})
    bits('CANCTRL','XF',{7:'REQOP2',6:'REQOP1',5:'REQOP0',4:'ABAT',3:'OSM',2:'CLKEN'})
    bits('CANSTAT','XE',{7:'OPMOD2',6:'OPMOD1',5:'OPMOD0'})
    bits('CNF1','2A',{7:'SJW1',6:'SJW0',5:'BRP5',0:'BRP0'})
    bits('CNF2','29',{7:'BTLMODE',6:'SAM',5:'PHSEG12',3:'PHSEG10',2:'PRSEG2',0:'PRSEG0'})
    bits('CNF3','28',{2:'PHSEG22',0:'PHSEG20'})
    bits('TXB0CTRL','30',{6:'ABTF',3:'TXREQ',1:'TXP1',0:'TXP0'})
    bits('TXB2CTRL','50',{3:'TXREQ'})
    bits('EFLG','2D',{7:'RX1OVR',6:'RX0OVR',5:'TXBO'})
    for lab,num,k in [('RESET C0',r'1100 0000',['reset']),('READ 03',r'0000 0011',['read']),('WRITE 02',r'0000 0010',['write']),
        ('READ RX BUFFER 90',r'1001 0nm0',['read rx buffer']),('LOAD TX BUFFER 40',r'0100 0abc',['load tx buffer']),('RTS 80',r'1000 0nnn',['rts']),
        ('READ STATUS A0',r'1010 0000',['read status']),('RX STATUS B0',r'1011 0000',['rx status']),('BIT MODIFY 05',r'0000 0101',['bit modify'])]:
        chk('MCP2515',L,lab,num,k,0)
    for lab,num,k in [('REQOP 000 normal',r'000 = Sets Normal',['normal']),('REQOP 001 sleep',r'001 = Sets Sleep',['sleep']),('REQOP 010 loopback',r'010 = Sets Loopback',['loopback']),
        ('REQOP 011 listen-only',r'011 = Sets Listen',['listen']),('REQOP 100 configuration',r'100 = Sets Config',['configuration'])]:
        chk('MCP2515',L,lab,num,k,0)
    for lab,num,k in [('RXBnSIDL bit4 SRR',r'bit 4\s+SRR',['srr']),('RXBnSIDL bit3 IDE',r'bit 3\s+IDE',['ide']),('RXBnDLC bit6 RTR',r'bit 6\s+RTR',['rtr']),
        ('RXBnSIDH 61h/71h',r'ADDRESS: 61h',['rxbnsidh']),('RXBnSIDL 62h/72h',r'ADDRESS: 62h',['rxbnsidl']),('RXBnDLC 65h/75h',r'ADDRESS: 65h',['rxbndlc'])]:
        chk('MCP2515',L,lab,num,k,1)
    chk('MCP2515',L,'OST 128 OSC1 클럭',r'128 OSC1',['reset'],1)
    chk('MCP2515',L,'READ RX BUFFER 가 RXnIF 해제',r'automatically clearing',['rxnif'],2)
    chk('MCP2515',L,'5.5절 PropSeg+PS1 ≥ PS2',r'PropSeg \+ PS1',['ps2'],0)

# ---- nRF24L01+ (선택: nrf24.txt = Product Specification v1.0 의 pdftotext -layout 결과). Table 20 명령, Table 28 레지스터, Table 16 타이밍, Table 22 SPI ----
NP=os.path.join(D,'nrf24.txt')
if os.path.exists(NP):
    L=load(NP)
    for lab,num,k in [('R_REGISTER 000A AAAA',r'R_REGISTER\s+000A AAAA',[]),('W_REGISTER 001A AAAA',r'W_REGISTER\s+001A AAAA',[]),
        ('R_RX_PAYLOAD 61h',r'R_RX_PAYLOAD\s+0110 0001',[]),('W_TX_PAYLOAD A0h',r'W_TX_PAYLOAD\s+1010 0000',[]),('FLUSH_TX E1h',r'FLUSH_TX\s+1110 0001',[]),
        ('FLUSH_RX E2h',r'FLUSH_RX\s+1110 0010',[]),('REUSE_TX_PL E3h',r'REUSE_TX_PL\s+1110 0011',[]),('R_RX_PL_WID 60h',r'R_RX_PL_WID\w*\s+0110 0000',[]),
        ('W_ACK_PAYLOAD 1010 1PPP',r'W_ACK_PAYLOAD\w*\s+1010 1PPP',[]),('W_TX_PAYLOAD_NOACK B0h',r'1011 0000',[]),('NOP FFh',r'NOP\s+1111 1111',[])]:
        chk('nRF24',L,lab,num,k,0)
    for addr,name in [('00','CONFIG'),('01','EN_AA'),('02','EN_RXADDR'),('03','SETUP_AW'),('04','SETUP_RETR'),('05','RF_CH'),('06','RF_SETUP'),('07','STATUS'),
        ('08','OBSERVE_TX'),('09','RPD'),('0A','RX_ADDR_P0'),('0B','RX_ADDR_P1'),('0C','RX_ADDR_P2'),('0F','RX_ADDR_P5'),('10','TX_ADDR'),('11','RX_PW_P0'),('16','RX_PW_P5'),
        ('17','FIFO_STATUS'),('1C','DYNPD'),('1D','FEATURE')]:
        chk('nRF24',L,f'주소 {addr}h = {name}',r'^\s*'+addr+r'\s+'+name+r'\b',[],0)
    def bit(name,b,reset=None,label=None):
        pat=r'^\s*'+name+r'[a-z]?\s+'+b+r'\s+'+(reset if reset is not None else r'\S+')+r'\s'
        chk('nRF24',L,label or f'{name} 비트 {b}'+(f' 리셋 {reset}' if reset else ''),pat,[],0)
    for nm,b,r in [('MASK_RX_DR','6','0'),('MASK_TX_DS','5','0'),('MASK_MAX_RT','4','0'),('EN_CRC','3','1'),('CRCO','2','0'),('PWR_UP','1','0'),('PRIM_RX','0','0'),
        ('ENAA_P5','5','1'),('ENAA_P0','0','1'),('ERX_P5','5','0'),('ERX_P1','1','1'),('ERX_P0','0','1'),('AW','1:0','11'),('ARD','7:4','0000'),('ARC','3:0','0011'),
        ('RF_CH','6:0','0000010'),('CONT_WAVE','7','0'),('RF_DR_LOW','5','0'),('RF_DR_HIGH','3','1'),('RF_PWR','2:1','11'),
        ('RX_DR','6','0'),('TX_DS','5','0'),('MAX_RT','4','0'),('RX_P_NO','3:1','111'),('PLOS_CNT','7:4','0'),('ARC_CNT','3:0','0'),
        ('TX_REUSE','6','0'),('TX_EMPTY','4','1'),('RX_FULL','1','0'),('RX_EMPTY','0','1'),
        ('DPL_P5','5','0'),('DPL_P0','0','0'),('EN_DPL','2','0'),('EN_ACK_PAY','1','0'),('EN_DYN_ACK','0','0')]:
        bit(nm,b,r)
    chk('nRF24',L,'STATUS TX_FULL 비트 0',r'^\s*TX_FULL\s+0\s',[],0); chk('nRF24',L,'FIFO_STATUS TX_FULL 비트 5',r'^\s*TX_FULL\s+5\s',[],0)
    for lab,num,k in [('RF_DR 인코딩 10=250kbps',r"[‘'’]10[’']\s+\S\s*250kbps",[]),('RF_DR 인코딩 01=2Mbps',r"[‘'’]01[’']\s+\S\s*2Mbps",[]),('RF_DR 인코딩 00=1Mbps',r"[‘'’]00[’']\s+\S\s*1Mbps",[]),
        ('RF_PWR 11=0dBm',r"'11'\s+.\s*0dBm",[]),('RF_PWR 00=-18dBm',r"'00'\s+.\s*-18dBm",[]),('AW 01=3바이트',r"'01'\s+.\s*3 bytes",[]),('AW 11=5바이트',r"'11'\s+.\s*5 bytes",[]),
        ('ARD 0000=250µs',r'[‘\'’]0000[’\']\s+\S\s*Wait 250',[]),('ARD 1111=4000µs',r'[‘\'’]1111[’\']\s+\S\s*Wait 4000',[]),('RF_CH 식 f=2400+RF_CH',r'F0\s*=\s*2400\s*\+\s*RF_CH',[]),
        ('Tstby2a 130µs',r'Tstby2a.*130',[]),('Thce 10µs',r'Thce.*10',[]),('Tpece2csn 4µs',r'Tpece2csn.*4',[]),('Tpd2stby 1.5 ms',r'Tpd2stby.*1\.5ms',[]),('Tpd2stby 4.5 ms',r'^\s*4\.5ms\s',[]),
        ('SPI Fsck 10 MHz (Cload 5 pF 표)',r'Fsck\s+SCK Frequency\s+0\s+10\s+MHz',[]),('SPI Tcwh 50 ns',r'Tcwh\s+CSN Inactive time\s+50',[]),
        ('SPI 데이터 바이트 순서',r'LSByte to MSByte, MSBit in each byte first',[]),('W_REGISTER 대기/전원차단 모드에서만',r'Executable in power down or standby modes',[]),
        ('각주: FEATURE 비트 필요',r'bits in the FEATURE register',[]),('ACTIVATE 명령이 문서에 없음(음성 대조)',r'^$',[])]:
        if lab.startswith('ACTIVATE'):
            res.append(('nRF24',lab,not any('ACTIVATE' in l for l in L)))
        else: chk('nRF24',L,lab,num,k,0)

def facts_check(tag,hdr,facts,prefix):
    """사실표(def|이름|근거1 값|근거2 값|메모)와 헤더 #define 을 대조한다. 근거2 가 '-' 가 아니면 두 근거가 같은지도 본다.
    min|이름|최소값 은 헤더의 대기 시간 상수가 데이터시트 최소값 이상인지,
    val|설명|값1|값2 은 헤더에 매크로가 없는 항목(두 근거끼리만 비교), !val 은 알려진 불일치(실패로 세지 않음)."""
    defs={}
    for l in open(hdr,encoding='utf-8'):
        m=re.match(r'#define\s+(%s\w+)\s+(.*)$'%prefix,l)
        if not m: continue
        expr=re.sub(r'/\*.*?\*/','',m.group(2)).strip()
        expr=re.sub(r'(\b0[xX][0-9A-Fa-f]+|\b\d+)[uU]\b',r'\1',expr)
        try: defs[m.group(1)]=eval(expr,{'__builtins__':{}},{})
        except Exception: pass
    for l in open(facts,encoding='utf-8'):
        l=l.rstrip('\n')
        if not l.strip() or l.startswith('#'): continue
        parts=l.split('|'); kind=parts[0]
        if kind=='def':
            name,ds,nt=parts[1],int(parts[2],0),parts[3]
            res.append((tag,f'{name} = {parts[2]} (헤더 = 사실표)',defs.get(name)==ds))
            if nt!='-': res.append((tag,f'{name}: 근거 둘이 일치',int(nt,0)==ds))
        elif kind=='min':
            res.append((tag,f'{parts[1]} >= {parts[2]} (헤더 대기 상수가 데이터시트 최소값 이상)',defs.get(parts[1],-1)>=int(parts[2],0)))
        elif kind=='val':
            if parts[3]!='-': res.append((tag,f'{parts[1]}: 근거 둘이 일치',int(parts[2],0)==int(parts[3],0)))
        elif kind=='!val':
            res.append((tag+'(알려진 불일치, 실패로 세지 않음)',f'{parts[1]}: {parts[2]} / {parts[3]}',True))


# ---- W5500 (PDF 텍스트 대신 spi/ref/w5500_facts.txt: 데이터시트 v1.0.5 본문 요약과 NuttX 소스에서 옮긴 사실표) ----
# 이 환경에서는 W5500 PDF 를 직접 내려받지 못해 다른 블록처럼 "원문 텍스트 검색"이 아니다. 헤더의 상수를 사실표와 대조하고, 사실표의 두 출처(DS, NuttX)가 서로 같은지 본다.
HERE=os.path.dirname(os.path.abspath(__file__))
WH=os.path.join(HERE,'..','spi','sw','w5500.h'); WF=os.path.join(HERE,'..','spi','ref','w5500_facts.txt')
if os.path.exists(WH) and os.path.exists(WF):
    facts_check('W5500',WH,WF,'W5500_')
    import subprocess,tempfile
    csrc=('#include <stdio.h>\n#include "w5500.h"\nint main(void){for(int n=0;n<8;n++)printf("%d %d %d %d %d\\n",'
          'W5500_CTRL(W5500_BSB_SOCK_REG(n),W5500_RWB_READ,W5500_OM_VDM),W5500_CTRL(W5500_BSB_SOCK_REG(n),W5500_RWB_WRITE,W5500_OM_VDM),'
          'W5500_CTRL(W5500_BSB_SOCK_TX(n),W5500_RWB_WRITE,W5500_OM_VDM),W5500_CTRL(W5500_BSB_SOCK_RX(n),W5500_RWB_READ,W5500_OM_VDM),W5500_CTRL(W5500_BSB_COMMON,W5500_RWB_READ,W5500_OM_VDM));return 0;}\n')
    try:
        with tempfile.TemporaryDirectory() as td:
            open(os.path.join(td,'m.c'),'w').write(csrc)
            subprocess.run(['cc','-I',os.path.dirname(WH),os.path.join(td,'m.c'),'-o',os.path.join(td,'m')],check=True,capture_output=True)
            rows=[tuple(map(int,x.split())) for x in subprocess.run([os.path.join(td,'m')],check=True,capture_output=True,text=True).stdout.splitlines()]
        ds=[(((4*n+1)<<3)|0,((4*n+1)<<3)|4,((4*n+2)<<3)|4,((4*n+3)<<3)|0,0) for n in range(8)]      # DS: BSB = 00001+4n / 00010+4n / 00011+4n, 제어 바이트 = BSB<<3 | RWB<<2 | OM
        nt=[((n<<5)|0x08|0x00,(n<<5)|0x08|0x04,(n<<5)|0x10|0x04,(n<<5)|0x18|0x00,0) for n in range(8)]  # NuttX 식
        res.append(('W5500','제어 바이트 8소켓 (DS 의 BSB 식)',rows==ds)); res.append(('W5500','제어 바이트 8소켓 (NuttX 의 식)',rows==nt))
    except Exception as e:
        res.append(('W5500','제어 바이트 대조(cc 실행 실패: %s)'%type(e).__name__,False))
# ---- 그 밖의 소자: 저장소 안의 사실표(spi/ref, gpio/ref, ...)와 헤더를 대조 ----
for tag,hdr,facts,prefix in [('MAX7219',('spi','sw','max7219.h'),('spi','ref','max7219_facts.txt'),'MAX7219_'),
                             ('AXI GPIO',('gpio','sw','axi_gpio.h'),('gpio','ref','axi_gpio_facts.txt'),'GPIO_'),
                             ('HD44780',('i2c','sw','hd44780.h'),('i2c','ref','hd44780_facts.txt'),'HD44780_'),
                             ('시계(AXI Timer)',('gpio','sw','basic_io.h'),('gpio','ref','basic_io_facts.txt'),'TMR_'),
                             ('아날로그',('gpio','sw','analog_sensors.h'),('gpio','ref','analog_sensors_facts.txt'),'AN_'),
                             ('SNTP',('ethernet','sw','ntp.h'),('ethernet','ref','ntp_facts.txt'),'NTP_'),
                             ('DNS',('ethernet','sw','dns.h'),('ethernet','ref','dns_facts.txt'),'DNS_'),
                             ('DHCP',('ethernet','sw','dhcp.h'),('ethernet','ref','dhcp_facts.txt'),'DH(?:CP|O)_'),
                             ('JSON',('ethernet','sw','json.h'),('ethernet','ref','json_facts.txt'),'JSON_(?:CTRL|HEX|ESC)_'),
                             ('HC-06',('uart','sw','hc06.h'),('uart','ref','hc06_facts.txt'),'HC06_'),
                             ('HM-10',('uart','sw','hm10.h'),('uart','ref','hm10_facts.txt'),'HM10_'),
                             ('SSD1306',('i2c','sw','ssd1306.h'),('i2c','ref','ssd1306_facts.txt'),'SSD1306_'),
                             ('스테퍼 28BYJ-48',('motor','sw','stepper.h'),('motor','ref','stepper_facts.txt'),'STEPPER_(?:(?:MOTOR|GEAR|HZ|PAT)_|PHASE)'),
                             ('RTTTL',('gpio','sw','rtttl.h'),('gpio','ref','rtttl_facts.txt'),'RTTTL_(?:NAME_M|DEFAULT_|OCT_|BPM_|A4_)')]:
    hp=os.path.join(HERE,'..',*hdr); fp=os.path.join(HERE,'..',*facts)
    if os.path.exists(hp) and os.path.exists(fp): facts_check(tag,hp,fp,prefix)
PDFTAGS={'MCP2515','PG079','PG090','PG096','PG142','PG153','nRF24'}
if FACTS_ONLY:
    n=len([r for r in res if r[0] in PDFTAGS])
    res=[r for r in res if r[0] not in PDFTAGS]
    print('(--facts-only: PDF 텍스트 블록 항목 %d개는 비교하지 않고 제외)'%n)
bad=[r for r in res if not r[2]]
for r in res: print('OK  ' if r[2] else 'MISS', r[0], r[1])
print(len(res),'항목 중 자동 대조 실패',len(bad))
sys.exit(1 if bad else 0)
