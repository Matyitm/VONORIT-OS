#include <stdint.h>
#include "net_inet.h"
#include "net.h"
#include "timer.h"
static const uint8_t lip[4]={10u,0u,2u,15u}, dip[4]={10u,0u,2u,3u};
static uint16_t next_sp=49152u;
static uint16_t alloc_sp(void){uint16_t p=next_sp++;if(next_sp>53247u)next_sp=49152u;return p;}
static int parse_ipv4(const char* s,uint8_t out[4]){
	uint32_t v=0u;uint8_t pt=0u,dg=0u;
	for(uint16_t i=0u;s[i]!=0;++i){
		if(s[i]=='.'){if(!dg||pt>=3u)return 0;out[pt++]=(uint8_t)v;v=0u;dg=0u;}
		else if(s[i]>='0'&&s[i]<='9'){v=v*10u+(uint32_t)(s[i]-'0');if(v>255u)return 0;dg=1u;}
		else return 0;
	}
	if(!dg||pt!=3u)return 0;
	out[3]=(uint8_t)v;return 1;
}
static int resolve_ip(const char* host,uint8_t out[4]){return parse_ipv4(host,out)?1:net_dns_resolve(host,out);}
static uint16_t csum(const uint8_t* d,uint32_t n){uint32_t s=0;for(uint32_t i=0;i+1<n;i+=2)s+=((uint32_t)d[i]<<8)|d[i+1];if(n&1)s+=((uint32_t)d[n-1]<<8);while(s>>16)s=(s&0xFFFFu)+(s>>16);return (uint16_t)~s;}
static uint16_t tcsum(const uint8_t s[4],const uint8_t d[4],const uint8_t* t,uint16_t l){uint32_t a=((uint32_t)s[0]<<8)|s[1];a+=((uint32_t)s[2]<<8)|s[3];a+=((uint32_t)d[0]<<8)|d[1];a+=((uint32_t)d[2]<<8)|d[3];a+=6u+(uint32_t)l;for(uint32_t i=0;i+1<l;i+=2)a+=((uint32_t)t[i]<<8)|t[i+1];if(l&1)a+=((uint32_t)t[l-1]<<8);while(a>>16)a=(a&0xFFFFu)+(a>>16);return (uint16_t)~a;}
static void eth_ip(uint8_t* p,const uint8_t dm[6],const uint8_t di[4],uint8_t pr,uint16_t pl){const uint8_t* sm=net_get_mac();for(uint8_t i=0;i<6;++i){p[i]=dm[i];p[6+i]=sm[i];}p[12]=8;p[13]=0;uint16_t tot=(uint16_t)(20u+pl);p[14]=0x45;p[15]=0;p[16]=(uint8_t)(tot>>8);p[17]=(uint8_t)tot;p[18]=0x11;p[19]=0x22;p[20]=0;p[21]=0;p[22]=64;p[23]=pr;p[24]=0;p[25]=0;for(uint8_t i=0;i<4;++i){p[26+i]=lip[i];p[30+i]=di[i];}uint16_t c=csum(&p[14],20u);p[24]=(uint8_t)(c>>8);p[25]=(uint8_t)c;}
int net_dns_resolve(const char* dom,uint8_t out[4]){
	uint8_t dm[6];if(!net_is_present()||!net_resolve_arp(dip,dm))return 0;
	uint8_t p[256]={0},r[512];uint16_t q=54u,i=0u;p[42]=0x12;p[43]=0x34;p[44]=1;p[47]=1;
	while(dom[i]&&q<200u){uint16_t lp=q++,ln=0;while(dom[i]&&dom[i]!='.'&&q<210u){p[q++]=(uint8_t)dom[i++];++ln;}p[lp]=(uint8_t)ln;if(dom[i]=='.')++i;}
	p[q++]=0;p[q++]=0;p[q++]=1;p[q++]=0;p[q++]=1;uint16_t ul=(uint16_t)(q-34u);eth_ip(p,dm,dip,17u,ul);
	p[34]=0xD4;p[35]=0x31;p[36]=0;p[37]=53;p[38]=(uint8_t)(ul>>8);p[39]=(uint8_t)ul;p[40]=0;p[41]=0;net_send_frame(p,q);
	for(uint64_t end=timer_get_ticks()+250u;timer_get_ticks()<end;){uint16_t n=net_poll_frame(r,512u);if(n>=58u&&r[12]==8&&r[13]==0&&r[23]==17&&r[42]==0x12&&r[43]==0x34&&(r[49]>0||r[48]>0)){uint16_t pos=54u;while(pos<n&&r[pos]!=0){if((r[pos]&0xC0u)==0xC0u){pos+=2u;break;}pos+=(uint16_t)(r[pos]+1u);if(pos<n&&r[pos]==0){++pos;break;}}pos+=4u;uint16_t an=(uint16_t)((r[48]<<8)|r[49]);for(uint16_t a=0;a<an&&pos+12u<=n;++a){if((r[pos]&0xC0u)==0xC0u)pos+=2u;else{while(pos<n&&r[pos]!=0)pos+=(uint16_t)(r[pos]+1u);++pos;}if(pos+10u>n)break;uint16_t tp=(uint16_t)((r[pos]<<8)|r[pos+1]),dl=(uint16_t)((r[pos+8]<<8)|r[pos+9]);pos+=10u;if(tp==1u&&dl==4u&&pos+4u<=n){for(uint8_t k=0;k<4;++k)out[k]=r[pos+k];return 1;}pos+=dl;}}}return 0;
}
static void send_tcp(const uint8_t dm[6],const uint8_t di[4],uint16_t sp,uint16_t dp,uint32_t sq,uint32_t ak,uint8_t fl,const uint8_t* dt,uint16_t dl){uint8_t p[512]={0};uint16_t tl=(uint16_t)(20u+dl);eth_ip(p,dm,di,6u,tl);p[34]=(uint8_t)(sp>>8);p[35]=(uint8_t)sp;p[36]=(uint8_t)(dp>>8);p[37]=(uint8_t)dp;for(int k=0;k<4;++k){p[38+k]=(uint8_t)(sq>>(24-k*8));p[42+k]=(uint8_t)(ak>>(24-k*8));}p[46]=0x50;p[47]=fl;p[48]=0x20;p[49]=0;for(uint16_t i=0;i<dl;++i)p[54+i]=dt[i];uint16_t c=tcsum(lip,di,&p[34],tl);p[50]=(uint8_t)(c>>8);p[51]=(uint8_t)c;net_send_frame(p,(uint16_t)(34u+tl));}
int net_http_get(const char* host,char* out,uint16_t max){
	uint8_t di[4],dm[6];if(!resolve_ip(host,di)||!net_resolve_arp(di,dm))return 0;
	uint16_t sp=alloc_sp();uint32_t sq=1000u,ak=0u;uint8_t r[1024];send_tcp(dm,di,sp,80u,sq,0u,0x02u,0,0);uint8_t syn=0;
	for(uint64_t end=timer_get_ticks()+250u;timer_get_ticks()<end;){uint16_t n=net_poll_frame(r,1024u);if(n>=54u&&r[12]==8&&r[13]==0&&r[23]==6&&r[36]==(uint8_t)(sp>>8)&&r[37]==(uint8_t)sp&&(r[47]&0x12u)==0x12u){ak=((uint32_t)r[38]<<24)|((uint32_t)r[39]<<16)|((uint32_t)r[40]<<8)|r[41];++ak;++sq;syn=1;break;}}
	if(!syn)return 0;
	send_tcp(dm,di,sp,80u,sq,ak,0x10u,0,0);uint8_t req[160];uint16_t rl=0;const char* a="GET / HTTP/1.0\r\nHost: ";while(*a)req[rl++]=(uint8_t)*a++;for(uint16_t i=0;host[i]&&rl<120;++i)req[rl++]=(uint8_t)host[i];const char* b="\r\nConnection: close\r\n\r\n";while(*b)req[rl++]=(uint8_t)*b++;
	send_tcp(dm,di,sp,80u,sq,ak,0x18u,req,rl);sq+=rl;uint16_t op=0;
	for(uint64_t end=timer_get_ticks()+300u;timer_get_ticks()<end&&op+1<max;){uint16_t n=net_poll_frame(r,1024u);if(n>=54u&&r[12]==8&&r[13]==0&&r[23]==6&&r[36]==(uint8_t)(sp>>8)&&r[37]==(uint8_t)sp){uint16_t hl=(uint16_t)((r[46]>>4)*4u),ip_tot=(uint16_t)((r[16]<<8)|r[17]),pl=(uint16_t)(ip_tot>20u+hl?ip_tot-20u-hl:0u);if(pl>0u&&34u+hl+pl<=n){for(uint16_t k=0;k<pl&&op+1<max;++k){char ch=(char)r[34u+hl+k];if(ch!='\r')out[op++]=ch;}ak+=pl;send_tcp(dm,di,sp,80u,sq,ak,0x10u,0,0);}if(r[47]&1u)break;}}
	out[op]=0;return op>0?1:0;
}
int net_udp_chat(const char* host,uint16_t port,const char* msg,char* out_reply,uint16_t max_len){
	uint8_t di[4],dm[6];if(!net_is_present()||!resolve_ip(host,di)||!net_resolve_arp(di,dm))return -1;
	uint8_t p[512]={0},r[512];uint16_t sp=alloc_sp(),ml=0u;
	while(msg[ml]!=0&&ml<380u){p[42u+ml]=(uint8_t)msg[ml];++ml;}
	uint16_t ul=(uint16_t)(8u+ml);eth_ip(p,dm,di,17u,ul);
	p[34]=(uint8_t)(sp>>8);p[35]=(uint8_t)sp;p[36]=(uint8_t)(port>>8);p[37]=(uint8_t)port;p[38]=(uint8_t)(ul>>8);p[39]=(uint8_t)ul;p[40]=0;p[41]=0;
	net_send_frame(p,(uint16_t)(34u+ul));
	for(uint64_t end=timer_get_ticks()+150u;timer_get_ticks()<end;){uint16_t n=net_poll_frame(r,512u);if(n>=42u&&r[12]==8&&r[13]==0&&r[23]==17&&r[36]==(uint8_t)(sp>>8)&&r[37]==(uint8_t)sp){uint16_t rl=(uint16_t)((r[38]<<8)|r[39]);if(rl>8u&&34u+rl<=n&&out_reply&&max_len>1u){uint16_t op=0u,pl=(uint16_t)(rl-8u);for(uint16_t k=0u;k<pl&&op+1u<max_len;++k){char ch=(char)r[42u+k];if(ch!='\r')out_reply[op++]=ch;}out_reply[op]=0;return 1;}}}
	if(out_reply&&max_len>0u)out_reply[0]=0;
	return 0;
}
