#!/usr/bin/env python3
"""코드 헤더에 적은 레지스터 오프셋·비트 이름을 AMD PG 문서 텍스트(pdftotext -layout 결과)와 자동 대조한다.
사용: crosscheck_pdf.py <폴더>  (폴더에 pg079.txt pg090.txt pg096.txt pg142.txt pg153.txt). PDF 는 저장소에 넣지 않는다(재배포 문제).
한계: 이름과 숫자가 같은 줄/근처에 있는지만 본다. 비트 의미의 세부는 사람이 읽어 확인한 것이다."""
import re,sys
def load(p): return open(p,errors='ignore').read().splitlines()
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
import sys,os
D=sys.argv[1] if len(sys.argv)>1 else '.'
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
bad=[r for r in res if not r[2]]
for r in res: print('OK  ' if r[2] else 'MISS', r[0], r[1])
print(len(res),'항목 중 자동 대조 실패',len(bad))
sys.exit(1 if bad else 0)
