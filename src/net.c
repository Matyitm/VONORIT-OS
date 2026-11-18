#include <stdint.h>
#include "net.h"
#include "pci.h"
#include "timer.h"

struct rx_d{
uint64_t a;
uint16_t l,c;
uint8_t s,e;
uint16_t p;

}__attribute__((packed));

struct tx_d{
uint64_t a;
uint16_t l;
uint8_t o,c,s,u;
uint16_t p;

}__attribute__((packed));

static uint8_t nic_present,nic_link,is_e1000,mac[6];

static const char* nic_name="None";

static volatile uint32_t* mmio;

static struct rx_d rx_r[8] __attribute__((aligned(16)));

static struct tx_d tx_r[8] __attribute__((aligned(16)));

static uint8_t rx_b[8][2048] __attribute__((aligned(16)));

static uint8_t tx_b[8][2048] __attribute__((aligned(16)));

static uint16_t rx_t,tx_t;

static const uint8_t lip[4]={
10u,0u,2u,15u
};


static uint8_t inb(uint16_t p){
uint8_t v;
__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
return v;

}
static uint32_t mr(uint32_t o){
return mmio[o/4u];

}
static void mw(uint32_t o,uint32_t v){
mmio[o/4u]=v;

}
static uint16_t csum(const uint8_t* d,uint32_t n){
uint32_t s=0;
for(uint32_t i=0;
i+1<n;
i+=2)s+=((uint32_t)d[i]<<8)|d[i+1];
if(n&1)s+=((uint32_t)d[n-1]<<8);
while(s>>16)s=(s&0xFFFFu)+(s>>16);
return (uint16_t)~s;

}

static void setup_e1000(void){

 mw(0,mr(0)|(1u<<6));
for(uint32_t i=0;
i<128;
++i)mw(0x5200u+i*4u,0);

 for(uint16_t i=0;
i<8;
++i){
rx_r[i].a=(uint64_t)(uintptr_t)&rx_b[i];
rx_r[i].s=0;
tx_r[i].a=(uint64_t)(uintptr_t)&tx_b[i];
tx_r[i].c=0;
tx_r[i].s=1;

}
 rx_t=7;
tx_t=0;
uint64_t rb=(uint64_t)(uintptr_t)rx_r,tb=(uint64_t)(uintptr_t)tx_r;

 mw(0x2800u,(uint32_t)rb);
mw(0x2804u,(uint32_t)(rb>>32));
mw(0x2808u,sizeof(rx_r));
mw(0x2810u,0);
mw(0x2818u,7);
mw(0x0100u,(1u<<1)|(1u<<3)|(1u<<4)|(1u<<15)|(1u<<26));

 mw(0x3800u,(uint32_t)tb);
mw(0x3804u,(uint32_t)(tb>>32));
mw(0x3808u,sizeof(tx_r));
mw(0x3810u,0);
mw(0x3818u,0);
mw(0x0410u,0x0060200Au);
mw(0x0400u,(1u<<1)|(1u<<3)|(0x10u<<4)|(0x40u<<12));

 nic_link=1u;


}
static void send_raw(const uint8_t* f,uint16_t n){

 if(!is_e1000||!mmio||n>2000u)return;
uint16_t i=tx_t;
for(uint16_t k=0;
k<n;
++k)tx_b[i][k]=f[k];

 tx_r[i].l=n;
tx_r[i].c=0x0Bu;
tx_r[i].s=0;
tx_t=(uint16_t)((i+1u)&7u);
mw(0x3818u,tx_t);

 for(uint32_t s=0;
s<200000u;
++s)if(tx_r[i].s&1u)break;


}
static uint16_t poll_raw(uint8_t* o,uint16_t m){

 if(!is_e1000||!mmio)return 0;
uint16_t nx=(uint16_t)((rx_t+1u)&7u);
if(!(rx_r[nx].s&1u))return 0;

 uint16_t n=rx_r[nx].l;
if(n>m)n=m;
for(uint16_t k=0;
k<n;
++k)o[k]=rx_b[nx][k];
rx_r[nx].s=0;
rx_t=nx;
mw(0x2818u,rx_t);
return n;


}
void net_init(void){

 nic_present=0;
nic_link=0;
is_e1000=0;
mmio=0;
nic_name="None";
for(uint8_t i=0;
i<6;
++i)mac[i]=0;

 for(uint8_t i=0;
i<pci_count();
++i){
const pci_device_t* d=pci_device(i);
if(!d||d->class_code!=2u)continue;

  if(d->vendor_id==0x8086u&&d->device_id==0x100Eu){
pci_write16(d->bus,d->slot,d->func,4u,(uint16_t)(pci_read16(d->bus,d->slot,d->func,4u)|6u));

   if(!(d->bar0&1u)){
mmio=(volatile uint32_t*)((uintptr_t)(d->bar0&0xFFFFFFF0u));
uint32_t l=mr(0x5400u),h=mr(0x5404u);
mac[0]=l;
mac[1]=l>>8;
mac[2]=l>>16;
mac[3]=l>>24;
mac[4]=h;
mac[5]=h>>8;
is_e1000=1;
setup_e1000();

}
   nic_present=1;
nic_name="Intel E1000";
return;

}
  if(d->vendor_id==0x10ECu&&d->device_id==0x8139u){
pci_write16(d->bus,d->slot,d->func,4u,(uint16_t)(pci_read16(d->bus,d->slot,d->func,4u)|5u));

   uint16_t io=(uint16_t)(d->bar0&0xFFFFFFFCu);
if((d->bar0&1u)&&io)for(uint8_t j=0;
j<6;
++j)mac[j]=inb((uint16_t)(io+j));
nic_present=1;
nic_link=1;
nic_name="Realtek RTL8139";
return;

}
}

}
int net_ping_ipv4(const uint8_t tip[4],uint32_t* rtt){

 if(!nic_present||!is_e1000)return 0;
uint64_t t0=timer_get_ticks();
uint8_t p[96]={
0
},r[256],dm[6];

 for(uint8_t i=0;
i<6;
++i){
p[i]=0xFFu;
p[6+i]=mac[i];
p[22+i]=mac[i];

}p[12]=8;
p[13]=6;
p[15]=1;
p[16]=8;
p[18]=6;
p[19]=4;
p[21]=1;

 for(uint8_t i=0;
i<4;
++i){
p[28+i]=lip[i];
p[38+i]=tip[i];

}send_raw(p,42u);
uint8_t ok=0;

 for(uint32_t w=0;
w<400000u;
++w){
uint16_t n=poll_raw(r,256u);
if(n>=42u&&r[12]==8&&r[13]==6&&r[21]==2&&r[28]==tip[0]&&r[29]==tip[1]&&r[30]==tip[2]&&r[31]==tip[3]){
for(uint8_t i=0;
i<6;
++i)dm[i]=r[22+i];
ok=1;
break;

}
}
 if(!ok)return 0;
for(uint8_t i=0;
i<96;
++i)p[i]=0;
for(uint8_t i=0;
i<6;
++i){
p[i]=dm[i];
p[6+i]=mac[i];

}p[12]=8;
p[14]=0x45;
p[17]=0x3C;
p[18]=0x12;
p[22]=64;
p[23]=1;

 for(uint8_t i=0;
i<4;
++i){
p[26+i]=lip[i];
p[30+i]=tip[i];

}uint16_t ic=csum(&p[14],20u);
p[24]=ic>>8;
p[25]=ic;
p[34]=8;
p[39]=1;
p[41]=1;

 for(uint8_t i=0;
i<32;
++i)p[42+i]=(uint8_t)('A'+(i%26));
uint16_t cc=csum(&p[34],40u);
p[36]=cc>>8;
p[37]=cc;
send_raw(p,74u);

 for(uint32_t w=0;
w<500000u;
++w){
uint16_t n=poll_raw(r,256u);
if(n>=42u&&r[12]==8&&r[13]==0&&r[23]==1&&r[34]==0){
if(rtt)*rtt=(uint32_t)((timer_get_ticks()-t0)*10u+1u);
return 1;

}
}
 return 0;


}
uint8_t net_is_present(void){
return nic_present;

}const char* net_driver_name(void){
return nic_name;

}const uint8_t* net_get_mac(void){
return mac;

}uint8_t net_link_status(void){
return nic_link;

}
