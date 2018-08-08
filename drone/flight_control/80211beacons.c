#include "80211beacons.h"
#include <stdio.h>
#include <stdint.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <net/if.h>
#include <pcap.h>
#include <linux/filter.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>



#include <sys/ioctl.h>
#include <gcrypt.h>


#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/genetlink.h>
#include <linux/nl80211.h>



#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DEFAULT_PCAP_DEV "mon0"

#define COMMAND_LEN GCRY_PAYLOAD-CHECKSUM_LEN-SEQ_LEN
#define MAC_LEN 6
#define BEACONS_PER_REQUEST 1

#define BE_PAYLOAD_LEN 2046

pcap_t * capture;


uint64_t seq_num_tx=0;
uint64_t seq_num_rx=0;
uint64_t seq_num_rx_last=0;
uint32_t recvSocket=0;
                                                    //  00:10:7B:8A:55:4B
int8_t header_drone[]={ 0x00,0x00,0x0d,0x00,0x04,0x80,0x02,0x00,0x02,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
                        0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x10,0x7B,0x8A,0x55,0x4B,0x00,0x10,0x7B,0x8A,0x55,0x4B,
                        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x11,0x00,0x00};
                                                    //  00:10:7B:A4:64:46
int8_t header_pilot[]={0x00,0x00,0x0d,0x00,0x04,0x80,0x02,0x00,0x02,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
                        0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x10,0x7B,0xA4,0x64,0x46,0x00,0x10,0x7B,0xA4,0x64,0x46,
                        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x11,0x00,0x00};


int8_t * header1;
uint32_t header1_len;
    //https://www.kernel.org/doc/Documentation/networking/filter.txt
    //sudo tcpdump -i mon0 type mgt subtype beacon and \(ether host 66:66:66:66:66:66\) -dd

struct sock_filter code_drone[] = {    
    { 0x30, 0, 0, 0x00000003 },
    { 0x64, 0, 0, 0x00000008 },
    { 0x7, 0, 0, 0x00000000 },
    { 0x30, 0, 0, 0x00000002 },
    { 0x4c, 0, 0, 0x00000000 },
    { 0x2, 0, 0, 0x00000000 },
    { 0x7, 0, 0, 0x00000000 },
    { 0x50, 0, 0, 0x00000000 },
    { 0x54, 0, 0, 0x000000fc },
    { 0x15, 0, 33, 0x00000080 },
    { 0x50, 0, 0, 0x00000000 },
    { 0x45, 31, 0, 0x00000004 },
    { 0x45, 0, 21, 0x00000008 },
    { 0x50, 0, 0, 0x00000001 },
    { 0x45, 0, 9, 0x00000002 },
    { 0x45, 0, 4, 0x00000001 },
    { 0x40, 0, 0, 0x0000001a },
    { 0x15, 0, 12, 0x7ba46446 },
    { 0x48, 0, 0, 0x00000018 },
    { 0x15, 22, 10, 0x00000010 },
    { 0x40, 0, 0, 0x00000012 },
    { 0x15, 0, 16, 0x7ba46446 },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 18, 14, 0x00000010 },
    { 0x40, 0, 0, 0x0000000c },
    { 0x15, 0, 2, 0x7ba46446 },
    { 0x48, 0, 0, 0x0000000a },
    { 0x15, 14, 0, 0x00000010 },
    { 0x50, 0, 0, 0x00000001 },
    { 0x45, 0, 8, 0x00000001 },
    { 0x40, 0, 0, 0x00000012 },
    { 0x15, 0, 11, 0x7ba46446 },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 8, 9, 0x00000010 },
    { 0x40, 0, 0, 0x0000000c },
    { 0x15, 0, 2, 0x7ba46446 },
    { 0x48, 0, 0, 0x0000000a },
    { 0x15, 4, 0, 0x00000010 },
    { 0x40, 0, 0, 0x00000006 },
    { 0x15, 0, 3, 0x7ba46446 },
    { 0x48, 0, 0, 0x00000004 },
    { 0x15, 0, 1, 0x00000010 },
    { 0x6, 0, 0, 0x00040000 },
    { 0x6, 0, 0, 0x00000000 }
};

struct sock_filter code_pilot[] = {    
    { 0x30, 0, 0, 0x00000003 },
    { 0x64, 0, 0, 0x00000008 },
    { 0x7, 0, 0, 0x00000000 },
    { 0x30, 0, 0, 0x00000002 },
    { 0x4c, 0, 0, 0x00000000 },
    { 0x2, 0, 0, 0x00000000 },
    { 0x7, 0, 0, 0x00000000 },
    { 0x50, 0, 0, 0x00000000 },
    { 0x54, 0, 0, 0x000000fc },
    { 0x15, 0, 33, 0x00000080 },
    { 0x50, 0, 0, 0x00000000 },
    { 0x45, 31, 0, 0x00000004 },
    { 0x45, 0, 21, 0x00000008 },
    { 0x50, 0, 0, 0x00000001 },
    { 0x45, 0, 9, 0x00000002 },
    { 0x45, 0, 4, 0x00000001 },
    { 0x40, 0, 0, 0x0000001a },
    { 0x15, 0, 12, 0x7b8a554b },
    { 0x48, 0, 0, 0x00000018 },
    { 0x15, 22, 10, 0x00000010 },
    { 0x40, 0, 0, 0x00000012 },
    { 0x15, 0, 16, 0x7b8a554b },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 18, 14, 0x00000010 },
    { 0x40, 0, 0, 0x0000000c },
    { 0x15, 0, 2, 0x7b8a554b },
    { 0x48, 0, 0, 0x0000000a },
    { 0x15, 14, 0, 0x00000010 },
    { 0x50, 0, 0, 0x00000001 },
    { 0x45, 0, 8, 0x00000001 },
    { 0x40, 0, 0, 0x00000012 },
    { 0x15, 0, 11, 0x7b8a554b },
    { 0x48, 0, 0, 0x00000010 },
    { 0x15, 8, 9, 0x00000010 },
    { 0x40, 0, 0, 0x0000000c },
    { 0x15, 0, 2, 0x7b8a554b },
    { 0x48, 0, 0, 0x0000000a },
    { 0x15, 4, 0, 0x00000010 },
    { 0x40, 0, 0, 0x00000006 },
    { 0x15, 0, 3, 0x7b8a554b },
    { 0x48, 0, 0, 0x00000004 },
    { 0x15, 0, 1, 0x00000010 },
    { 0x6, 0, 0, 0x00040000 },
    { 0x6, 0, 0, 0x00000000 }

};
struct nl80211_state {

    struct nl_sock *nl_sock;
    struct nl_cache *nl_cache;
    struct genl_family *nl80211;
};
gcry_cipher_hd_t  gcry_hd;
char gcry_key[GCRY_KEYLEN]={0x0A,0xFA,0x12,0xA1,0x0A,0x55,0x66,0x8A,0x1F,0x1E,0x00,0x0D,0x0B,0xA5,0x55,0x01};

uint8_t device[10]=DEFAULT_PCAP_DEV;

enum beacons_op_mode op_mode;



struct nl80211_state nlstate;

uint16_t device_index=0;

static int linux_nl80211_init(struct nl80211_state *state, char * device_name)
{
    int err;

    state->nl_sock = nl_socket_alloc();

    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }

    if (genl_connect(state->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
        fprintf(stderr, "Failed to allocate generic netlink cache.\n");
        err = -ENOMEM;
        goto out_handle_destroy;
    }
    
    state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
    if (!state->nl80211) {
        fprintf(stderr, "nl80211 not found.\n");
        err = -ENOENT;
        goto out_cache_free;
    }
    device_index=if_nametoindex((char *)device_name);
    return 0;

 out_cache_free:
    nl_cache_free(state->nl_cache);
 out_handle_destroy:
    nl_socket_free(state->nl_sock);
    return err;
}
static void nl80211_cleanup(struct nl80211_state *state)
{
    genl_family_put(state->nl80211);
    nl_cache_free(state->nl_cache);
    nl_socket_free(state->nl_sock);
}

static uint8_t ieee80211_frequency_to_channel(uint32_t freq){
    if(freq>=2484){return 14;} 
    if(freq<2407){return 0;}
    return (freq-2407)/5;
}

static int ieee80211_channel_to_frequency(uint8_t chan){
    if (chan < 14)
        return 2407 + chan * 5;

    if (chan == 14)
        return 2484;

    /* FIXME: dot11ChannelStartingFactor (802.11-2007 17.3.8.3.2) */
    return (chan + 1000) * 5;
}
extern int16_t set_channel_nl80211(uint8_t channel){
    uint16_t freq;
    struct nl_msg *msg;

    uint16_t htval = NL80211_CHAN_NO_HT;
    freq=ieee80211_channel_to_frequency(channel);
    
    
    msg=nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "[E]failed to allocate netlink message\n");
        return 2;
    }

    genlmsg_put(msg, 0, 0, genl_family_get_id(nlstate.nl80211), 0,
            0, NL80211_CMD_SET_WIPHY, 0);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, device_index);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);

    nl_send_auto_complete(nlstate.nl_sock,msg);
    nlmsg_free(msg);
    //printf("\r\n[D]Changed channel to %i", channel);
    
    return( 0 );
 nla_put_failure:
    return -ENOBUFS;    


}

extern void print_vector(char * vector, uint16_t len){
        int ac=0;
        printf("\r\n");
        for(ac=0;ac<len;ac++){
                printf("%x ",vector[ac]&0x0000FF);
        }
        printf(" (%i)",len);
        fflush(stdout);
}

void random_vector(char * arr, int size){
        gcry_randomize (arr, size,GCRY_STRONG_RANDOM);
}
uint32_t openRecvSocket( const char device[IFNAMSIZ] )
{
    struct sock_fprog bpf;
    
    if(op_mode==BEACONS_INTERCEPTOR_PILOT){
        bpf.filter=code_pilot;
        bpf.len=ARRAY_SIZE(code_pilot);
    } else{
        bpf.filter=code_drone;
        bpf.len=ARRAY_SIZE(code_drone);    
    }   


    
    struct ifreq ifr;
    struct sockaddr_ll ll;
    const int protocol = ETH_P_ALL;
    int sock = -1;

    //assert( sizeof( ifr.ifr_name ) == IFNAMSIZ );

    sock = socket( PF_PACKET, SOCK_RAW, htons(protocol) );
    if ( sock < 0 )
    {
            perror( "socket failed (do you have root priviledges?)" );
            return -1;
    }
    int8_t ret = setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
    
    if(ret<0){
        perror( "Failed to apply filter" );
        return -1;
    }
    
    memset( &ifr, 0, sizeof( ifr ) );
    strncpy( ifr.ifr_name, device, sizeof(ifr.ifr_name) );
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
    {
            perror("ioctl[SIOCGIFINDEX]");
            close(sock);
            return -1;
    }

    memset( &ll, 0, sizeof(ll) );
    ll.sll_family = AF_PACKET;
    ll.sll_ifindex = ifr.ifr_ifindex;
    ll.sll_protocol = htons(protocol);
    if ( bind( sock, (struct sockaddr *) &ll, sizeof(ll) ) < 0 ) {
            perror( "bind[AF_PACKET]" );
            close( sock );
            return -1;
    }

    // Enable promiscuous mode
/*	struct packet_mreq mr;
    memset( &mr, 0, sizeof( mr ) );

    mr.mr_ifindex = ll.sll_ifindex;
    mr.mr_type    = PACKET_MR_PROMISC;

    if( setsockopt( sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof( mr ) ) < 0 )
    {
            perror( "setsockopt[PACKET_MR_PROMISC]" );
            close( sock );
            return -1;
    }
    */
    return sock;
}

extern int8_t init_80211beacons(enum beacons_op_mode mode){
    
    char errorBuffer[PCAP_ERRBUF_SIZE];
    int16_t result=0;
    int8_t rfmon_avail=0;
    if (!gcry_check_version (GCRYPT_VERSION)){
        return -1;         
    }   
    
    op_mode=mode;
    
    gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
    gcry_cipher_open(&gcry_hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, 0); 
    gcry_cipher_setkey (gcry_hd,gcry_key,GCRY_KEYLEN);
    pcap_if_t * allAdapters=NULL;
    pcap_if_t * adapter=NULL;  

    result= pcap_findalldevs(&allAdapters, errorBuffer );

    if(result==0){

        for( adapter = allAdapters; adapter != NULL; adapter = adapter->next)
        {
                printf("\r\n[I]Detected device %s", adapter->name);fflush(stdout);      
                if (strcmp(adapter->name,(const char *)device)==0){
                    rfmon_avail=1;                 
                    break;
                }

        }
    }
    
    if(rfmon_avail>0){
    printf("\n[I]RFMON device loaded: %s",device );fflush(stdout);      
    capture = pcap_open_live((const char *)adapter->name, BE_PAYLOAD_LEN, 0, 10, errorBuffer);
    
    printf("\n[I]Errors detected: %s",errorBuffer);fflush(stdout);   
    
    if(capture==NULL){printf("\r\n[E]NULL HANDLER");fflush(stdout);}
    
    recvSocket = openRecvSocket( adapter->name );
    
    linux_nl80211_init(&nlstate,adapter->name);
    
    
    
    }else{
         printf("\n[W]RFMON device not found: %s",device );fflush(stdout);     
    }
    
    
    
    
    header1_len=sizeof(header_drone);
    
    header1=header_drone;

    if(op_mode==BEACONS_INTERCEPTOR_PILOT){
        header1=header_pilot;
        header1_len=sizeof(header_pilot);
    } 
    
    
    
    return rfmon_avail;

}
extern void close_80211beacons(){
    nl80211_cleanup(&nlstate);
    close(recvSocket);
    pcap_close(capture);
}

void inject_command(pcap_t *fp, gcry_cipher_hd_t * hd,int8_t * ssid_tx, int8_t * command, uint16_t len, uint64_t * seqnum){
        char pkt[BE_PAYLOAD_LEN];
        u_char buf[BE_PAYLOAD_LEN];
        u_char iv[BE_PAYLOAD_LEN]={0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
        len+=SEQ_LEN;


        
        
        uint32_t tlen=len+CHECKSUM_LEN+GCRY_IVLEN;

        random_vector((char *)iv,GCRY_IVLEN);
        
        memcpy(&pkt[0],iv,GCRY_IVLEN);
        gcry_cipher_setiv(*hd,iv, GCRY_IVLEN);
   
                
          

        uint8_t cmdseq[BE_PAYLOAD_LEN];
        memcpy(cmdseq,command,len-SEQ_LEN);
        u_int ac=0;
        (*seqnum)++;
                   //FFFFFFFFFFFFFFFF
        //if(*seqnum>0x000000FFFFFFFFFF){*seqnum=0x01;}
        *seqnum=*seqnum&0x000000FFFFFFFFFF;
        for(ac=0;ac<SEQ_LEN;ac++){
                cmdseq[len-1-ac]=(uint8_t)(((*seqnum)>>(ac*8))&0x00000000000000FF);
                                                             //FFFFFFFFFFFFFFFF           
        }

        gcry_cipher_encrypt(*hd, &pkt[GCRY_IVLEN],len , cmdseq, len);
         u_char chkarr[CHECKSUM_LEN];
 
        gcry_md_hash_buffer(GCRY_MD_SHA256, chkarr, (uint8_t *)pkt, tlen-CHECKSUM_LEN);      
        memcpy(&pkt[GCRY_IVLEN+len], chkarr,CHECKSUM_LEN);

        
        int8_t head_size=header1_len;
        memcpy(buf, header1, head_size);
        

        buf[head_size]=(u_char)strlen((const char *)ssid_tx);
        head_size+=1;
        
        memcpy(buf+head_size, ssid_tx, strlen((const char *)ssid_tx));
        head_size+=(uint8_t)strlen((const char *)ssid_tx);
        
        buf[head_size]=(u_char)(tlen>>16)&0x000000FF;
        head_size+=1;
        buf[head_size]=(u_char)(tlen>>8)&0x000000FF;
        head_size+=1;
        buf[head_size]=(u_char)(tlen)&0x000000FF;
        head_size+=1;        

        memcpy(buf+head_size, pkt, tlen);
        
        //print_vector(pkt, tlen);

        for(ac=0;ac<BEACONS_PER_REQUEST;ac++){
            pcap_inject(fp,buf, head_size+tlen);
        }
        
}
extern void send_80211beacon(int8_t * ssid_tx,int8_t * stream, uint16_t len){
    int8_t query[COMMAND_LEN]="";
    uint16_t ac=0;
    for(ac=0;ac<COMMAND_LEN;ac++){
        if(ac<len){
            query[ac]=stream[ac];
        }else{
            query[ac]=0x00;
        }
    }

    inject_command(capture,&gcry_hd,ssid_tx,query, COMMAND_LEN, &seq_num_tx);
    
}


int8_t decode_packet(gcry_cipher_hd_t * hd,  int8_t * packet, uint32_t len, int8_t *command, uint32_t *commandlen, uint64_t * seqnum){
	
    uint8_t chk_carried[CHECKSUM_LEN];
    uint8_t chk_computed[CHECKSUM_LEN];
    memcpy(chk_carried,&packet[len-CHECKSUM_LEN],CHECKSUM_LEN);
    
    gcry_md_hash_buffer(GCRY_MD_SHA256, chk_computed, (uint8_t *)packet,len-CHECKSUM_LEN);

    
    if(memcmp(chk_computed,chk_carried,CHECKSUM_LEN)!=0){
        printf("\r\n BAD SHA256. PKT LEN= %i",len);
        
        return -1;
    }

    gcry_cipher_setiv(*hd,&packet[0], GCRY_IVLEN);

    *commandlen=len-GCRY_IVLEN-CHECKSUM_LEN;
    gcry_cipher_decrypt (*hd,command,*commandlen ,&packet[GCRY_IVLEN], *commandlen);    
    
    uint64_t ac=0;
    uint64_t cseqnum=0;
    for(ac=0;ac<SEQ_LEN;ac++){                     //FFFFFFFFFFFFFFFF     
            cseqnum|=((command[(*commandlen-1-ac)]&0x00000000000000FF)<<(ac*8));
    
    }
    //printf("\r\n sec %lli %li",cseqnum,*commandlen);fflush(stdout);
    if(*seqnum>=cseqnum){
        printf("\r\n OUT OF ORDER! %lli",cseqnum );fflush(stdout);
        //return -2;
    }
    if((((int8_t)(cseqnum>>54))!=0x00)||((((int8_t)(cseqnum>>46))!=0x00))){
        printf("\r\n *commandlen-1-ac %lli %x  seq %x",*commandlen-1-ac,command[(*commandlen-1-ac)],cseqnum);
        return -3;
    }
    *seqnum=cseqnum;
    *commandlen-=SEQ_LEN;

    return 0;
}
void packet_handler(const uint8_t *pkt_data, int32_t caplen,int8_t * command, int32_t * command_len, int64_t * seqnum, uint8_t *radiotap_channel){
    //int8_t command_o[GCRY_PAYLOAD];
    //uint32_t commandlen=0;
    *command_len=0;
    if(caplen<GCRY_PAYLOAD+30){return;}    
    if((pkt_data[0]!=0x00)||(pkt_data[1]!=0x00)){return ;}
    
    uint8_t hlen=(uint8_t)pkt_data[2]+24+13;
    
    *radiotap_channel=ieee80211_frequency_to_channel((pkt_data[27]<<8)|(pkt_data[26]));
    //printf("\r\n rido %i %x ",*radiotap_channel,(pkt_data[27]<<8)|(pkt_data[26]));fflush(stdout);
    uint32_t grcy_len_pos=hlen+pkt_data[hlen]+1;    
    uint32_t grcy_len=(pkt_data[grcy_len_pos]<<16)|(pkt_data[grcy_len_pos+1]<<8)|(pkt_data[grcy_len_pos+2]);

    uint32_t gcry_start=grcy_len_pos+3;
    uint32_t gcry_end=gcry_start+grcy_len;

    if((gcry_end>gcry_start)&&(gcry_end<=caplen)){
        if(decode_packet(&gcry_hd,(int8_t *)&pkt_data[gcry_start],(uint32_t)grcy_len,(int8_t *)command,(uint32_t *) command_len,(uint64_t*)&seq_num_rx)==0){
            //printf("\r\n SENT %s (%i) ->%llu \t",command, (uint16_t)strlen((char *)command),seq_num_rx);fflush(stdout);
            
            *seqnum=seq_num_rx;
            seq_num_rx_last=seq_num_rx;
        }else{
            *command_len=0;
        }    
    }

}

extern void recv_80211beacon(int8_t * command, int32_t * command_len, int64_t * seqnum,uint8_t * radiotap_channel){
    
    uint8_t pkt_data[4096];
    fd_set readfds;
    FD_ZERO( &readfds );
    FD_SET( recvSocket, &readfds );
    struct timeval tout;
    tout.tv_sec=0;
    tout.tv_usec=1000; //1ms
;
    int16_t numFds = select( recvSocket+1, &readfds, NULL, NULL, &tout );

            if (numFds == 1){
                uint32_t caplen = read( recvSocket,  pkt_data, sizeof( pkt_data) );    
                if(pkt_data!=NULL){
                packet_handler(pkt_data,caplen,command, command_len,seqnum,radiotap_channel);
            }
    }
}

extern void reset_recv_seqnum(){
    seq_num_rx=0;
    seq_num_rx_last=0;

}