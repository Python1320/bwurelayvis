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
		throw std::runtime_error(  "rx failed" );
	}
	
	uint64_t ret = read_u64(fp);
	return ret;
}
uint64_t BWReader::ReadTX() 
{  
    FILE* fp;
	fp = fopen(tx_path, "r");
	
		
	if (fp==nullptr) {
		throw std::runtime_error(  "tx failed" );
	}
	
	uint64_t ret = read_u64(fp);
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
	
	ret /= 0x20000; // to Mbps
	ret /= (double)(rx_speed);
	ret *= 100;
	return ret;
}

double BWReader::PercentageTX()
{
	uint64_t diff = last_tx-tx1;
	
	
	diff /= sample_time;
	double ret = (double)diff;
	
	ret /= 0x20000; // to Mbps
	ret *= 100;
	ret /= (double)(tx_speed);
	return ret;
}


int main(int argc, char *argv[])
{
	const char * str = getCmdOption(argv, argv + argc, "-i");
	const char * iface_name = str?str:"eth0";
	
	if (argc<=1) {
		printf(
			"Options:\n"
			" -i iface     Measurement interface\n"
			" -d delay     Delay between measurements\n"
			" -rx num      Maximum RX speed (Mbps)\n"
			" -tx num      Maximum TX speed (Mbps)\n"
			" -o iface     Output Bind IP\n"
			" -h hosts     Output interfaces, separate with \",\"\n"
			" -p ports     Output ports, separate with \",\"\n"
		);
		return 1;
	}
	str = getCmdOption(argv, argv + argc, "-d");
	int delay=1;
	if (str) {
		delay = std::stoi(str);
	}

	str = getCmdOption(argv, argv + argc, "-rx");
	int rxs=100;
	if (str) {
		rxs = std::stoi(str);
	}
	str = getCmdOption(argv, argv + argc, "-tx");
	int txs=100;
	if (str) {
		txs = std::stoi(str);
	}

	printf("Reading %s every %d seconds. Max RX: %dMbps. Max TX: %dMbps\n",iface_name,delay,rxs,txs);
	BWReader iface(iface_name,delay,rxs,txs);
	while (true) {
		iface.MeasureRXTX();
		float prx = (float)iface.PercentageRX();
		float ptx = (float)iface.PercentageTX();
		printf("rx: %.4f, tx: %.4f\n",prx,ptx);
	}

	return 0;
}
