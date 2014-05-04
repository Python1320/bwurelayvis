#include "main.h"

class BWReader                   
{
  public:                    
    BWReader(const char * _iface,int _sample_time,unsigned int rx_speed,unsigned int tx_speed);     
    ~BWReader();
    void MeasureRXTX();
	double PercentageRX();
	double PercentageTX();
	uint64_t ReadTX();
	uint64_t ReadRX();
 private:
    
	int sample_time;
	unsigned int rx_speed;
	unsigned int tx_speed;
	
	const char * iface;
	char rx_path[256];
	char tx_path[256];
	
	
	uint64_t rx1;
	uint64_t tx1;
	uint64_t last_rx = 0;
	uint64_t last_tx = 0;
	
	struct timespec tim;
	
	uint64_t read_u64(FILE* fp);
	
};

 
BWReader::BWReader(const char * iface,int sample_time,unsigned int rx_speed,unsigned int tx_speed)
{
	this->iface = iface;
	this->sample_time = sample_time;
	this->rx_speed = rx_speed;
	this->tx_speed = tx_speed;

	sprintf(rx_path,"/sys/class/net/%s/statistics/rx_bytes",iface);
	sprintf(tx_path,"/sys/class/net/%s/statistics/tx_bytes",iface);
	
	if( access( rx_path, F_OK ) != -1 ) {
		// ok?
	} else {
		throw std::runtime_error(  "interface could not be read" );
	}


	tim.tv_sec = sample_time;
	tim.tv_nsec = 0;
	
}

BWReader::~BWReader()                 
{
}


uint64_t BWReader::read_u64(FILE* fp) {
	uint64_t ret = 0;
	fseek(fp, 0, SEEK_SET);
	char buffer[64];
	
	if (fgets(buffer, 64, fp)!=NULL) {
		ret = strtoull(buffer, (char **)NULL, 0);
	}
	
	//printf("ret: %llu\n",ret);
	
	return ret;
}


uint64_t BWReader::ReadRX() 
{  
    FILE* fp;
	fp = fopen(rx_path, "r");
	if (fp==nullptr) {
		perror("fopen rx");
		throw std::runtime_error(  "rx failed" );
	}
	
	uint64_t ret = read_u64(fp);
	
	fclose(fp);
	
	return ret;
}
uint64_t BWReader::ReadTX() 
{  
    FILE* fp;
	fp = fopen(tx_path, "r");
	
		
	if (fp==nullptr) {
		perror("fopen tx");
		throw std::runtime_error(  "tx failed" );
	}
	
	uint64_t ret = read_u64(fp);
	
	fclose(fp);
	
	return ret;
}


void BWReader::MeasureRXTX() {
	struct timespec tim2;
	
	rx1 = ReadRX();
	tx1 = ReadTX();

	if(nanosleep(&tim , &tim2) < 0 )   
	{
		throw std::runtime_error(  "nanosleep failed" );
	}

	last_rx = ReadRX();
	last_tx = ReadTX();
	
	//printf("rx1: %llu, last_rx: %llu, tx1: %llu, last_tx: %llu\n",rx1,last_rx,tx1,last_tx);
	
}

double BWReader::PercentageRX() 
{
	uint64_t diff = last_rx-rx1;
	
	
	diff /= sample_time;
	double ret = (double)diff;
	
	ret /= 0x20000; // to_addr Mbps
	ret /= (double)(rx_speed);
	ret *= 100;
	return ret;
}

double BWReader::PercentageTX()
{
	uint64_t diff = last_tx-tx1;
	
	
	diff /= sample_time;
	double ret = (double)diff;
	
	ret /= 0x20000; // to_addr Mbps
	ret *= 100;
	ret /= (double)(tx_speed);
	return ret;
}


int main(int argc, char *argv[])
{
	
	if (argc!=7) {
		printf(
			"Usage: %s iface delay rx tx ip port\n"
			"delay  seconds between measurements\n"
			"rx,tx  maximum interface rx/tx speed (Mbps)\n"
			"ip     remote ip\n"
			"port   remote port\n"
			,argv[0]);
		return 1;
	}
	
	const char * iface_name = argv[1];
	
	const char * str = argv[2];
	int delay=1;
	if (str) {
		delay = std::stoi(str);
	}

	str = argv[3];
	int rxs=100;
	if (str) {
		rxs = std::stoi(str);
	}
	str = argv[4];
	int txs=100;
	if (str) {
		txs = std::stoi(str);
	}


	const char * ip = argv[5];
	const char * port = argv[6];

	int	sd, ret;
	struct	sockaddr_in to_addr;
	
	sd = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sd < 0 ) 
	{
		throw std::runtime_error(  "socket create failed" );
	}

	memset( &to_addr, 0, sizeof(to_addr) );
	to_addr.sin_family = AF_INET;
	to_addr.sin_port = htons(atoi(port));
	to_addr.sin_addr.s_addr = inet_addr(ip);

	char sendbuf[32];
	socklen_t structlen;



	printf("Reading %s every %d seconds. Max RX: %dMbps. Max TX: %dMbps\n",iface_name,delay,rxs,txs);
	BWReader iface(iface_name,delay,rxs,txs);
	while (true) {
		iface.MeasureRXTX();
		double rxp = iface.PercentageRX();
		double txp = iface.PercentageTX();
		
		printf("rx: %.3f%%, tx: %.3f%%\n",rxp,txp);
		sendbuf[0]=(char)round(rxp>120?120:rxp<0?0:rxp);
		sendbuf[1]=(char)round(txp>120?120:txp<0?0:txp);
		ret = sendto(sd,sendbuf,2,0,(struct sockaddr *)&to_addr,sizeof(to_addr));
		if ( ret < 0 )
		{
			printf("Send fail: %d\n",ret);
		}
	}

	return 0;
}
