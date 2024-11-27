#include <stdbool.h>
#include <iio/iio.h>
#include <iio/iio-debug.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x * 1000000000.0 + .5))


#define BLOCK_SIZE (1024*1024)


enum iodev { RX, TX };

struct stream_cfg {
  long long bw_hz;
  long long fs_hz;
  long long lo_hz;
  const char* rfport;
};

static struct iio_context *ctx   = NULL;
static struct iio_channel *rx0_i = NULL;
static struct iio_channel *rx0_q = NULL;
static struct iio_channel *tx0_i = NULL;
static struct iio_channel *tx0_q = NULL;
static struct iio_buffer  *rxbuf = NULL;
static struct iio_buffer  *txbuf = NULL;
static struct iio_stream  *rxstream = NULL;
static struct iio_stream  *txstream = NULL;
static struct iio_channels_mask *rxmask = NULL;
static struct iio_channels_mask *txmask = NULL;


void write_data(const char *fname, int16_t *I, int16_t *Q, int n) {
  FILE *samples;
  samples = fopen(fname, "w");
  
  for (int i = 0; i<n; i++) {
    fprintf(samples,"%d %d", I[i], Q[i]);
    fprintf(samples, "\n");

  }
   
    fclose(samples);
}

void read_data(const char *fname, int16_t *I, int16_t *Q, int n) {
  FILE *samples;
  samples = fopen(fname, "r");

  for (int i = 0; i<n; i++) {
    double a, b;
    fscanf(samples, "%lf %lf", &a, &b);
    I[i] = (int) a;
    Q[i] = (int) b;
      }
  fclose(samples);
  
}



static void shutdown()
{
	printf("* Destroying streams\n");
	if (rxstream) {iio_stream_destroy(rxstream); }
	if (txstream) {iio_stream_destroy(txstream); }

	printf("* Destroying buffers\n");
	if (rxbuf) { iio_buffer_destroy(rxbuf); }
	if (txbuf) { iio_buffer_destroy(txbuf); }

	printf("* Destroying channel masks\n");
	if (rxmask) { iio_channels_mask_destroy(rxmask); }
	if (txmask) { iio_channels_mask_destroy(txmask); }

	printf("* Destroying context\n");
	if (ctx) { iio_context_destroy(ctx); }

}

void set_dev(struct iio_device **phy, struct iio_device **tx,
             struct iio_device **rx) {
  *tx = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
  *rx = iio_context_find_device(ctx, "cf-ad9361-lpc");
  *phy = iio_context_find_device(ctx, "ad9361-phy");
}

void set_param(struct iio_channel *chn, struct iio_channel *lo_chn, struct iio_device *phy, struct stream_cfg cfg,enum iodev type) {
  switch (type) {
    
  case TX:{
    printf("* Настройка параметров %s канала AD9361 \n", "TX");
    chn = iio_device_find_channel(phy, "voltage0", true);
    printf("....");
    const struct iio_attr *tx_rf_port_attr = iio_channel_find_attr(chn, "rf_port_select");
    iio_attr_write_string(tx_rf_port_attr, cfg.rfport);
    const struct iio_attr *tx_bw_attr = iio_channel_find_attr(chn, "rf_bandwidth");
    iio_attr_write_longlong(tx_bw_attr, cfg.bw_hz);
    const struct iio_attr *tx_fs_attr = iio_channel_find_attr(chn, "sampling_frequency");
    iio_attr_write_longlong(tx_fs_attr, cfg.fs_hz);
    
    printf("* Настройка частоты опорного генератора (lo, local oscilator)  %s \n", "TX");
    lo_chn = iio_device_find_channel(phy, "altvoltage1", true);
    const struct iio_attr *tx_lo_attr = iio_channel_find_attr(lo_chn, "frequency");
    iio_attr_write_longlong(tx_lo_attr, cfg.lo_hz);
    const struct iio_attr *tx_gain_attr = iio_channel_find_attr(chn, "hardwaregain");
    iio_attr_write_longlong(tx_gain_attr, 70);}
    break;
    
  case RX:{
    printf("* Настройка параметров %s канала AD9361 \n", "RX");
    chn = iio_device_find_channel(phy, "voltage0", false);
    const struct iio_attr *rx_rf_port_attr = iio_channel_find_attr(chn, "rf_port_select");
    iio_attr_write_string(rx_rf_port_attr, cfg.rfport);
    const struct iio_attr *rx_bw_attr = iio_channel_find_attr(chn, "rf_bandwidth");
    iio_attr_write_longlong(rx_bw_attr, cfg.bw_hz);
    const struct iio_attr *rx_fs_attr = iio_channel_find_attr(chn, "sampling_frequency");
    iio_attr_write_longlong(rx_fs_attr, cfg.fs_hz);
    printf("* Настройка частоты опорного генератора (lo, local oscilator)  %s \n", "RX");
    lo_chn = iio_device_find_channel(phy, "altvoltage0", true);
    
    const struct iio_attr *rx_lo_attr = iio_channel_find_attr(lo_chn, "frequency");
    iio_attr_write_longlong(rx_lo_attr, cfg.lo_hz);
     const struct iio_attr *rx_gain_attr = iio_channel_find_attr(chn, "gain_control_mode");
     iio_attr_write_string(rx_gain_attr, "slow_attack");}
    break;

  }
 
}

int main() {
  printf("T_T\n");
  ctx = iio_create_context(NULL, "ip:192.168.3.1");
  printf("Hello\n");
 
  struct iio_device *phy_dev;
  struct iio_device *tx_dev;
  struct iio_device *rx_dev;

  struct stream_cfg txcfg, rxcfg;

  rxcfg.bw_hz = MHZ(10);
  rxcfg.fs_hz = MHZ(10);
  rxcfg.lo_hz = GHZ(1);
  rxcfg.rfport = "A_BALANCED";

  txcfg.bw_hz = MHZ(10);
  txcfg.fs_hz = MHZ(10);
  txcfg.lo_hz = GHZ(1);
  txcfg.rfport = "A_BALANCED";
  
  
  set_dev(&phy_dev, &tx_dev, &rx_dev);

  

  struct iio_channel *tx_chn = NULL;
  struct iio_channel *tx_lo_chn = NULL;
  
  struct iio_channel *rx_chn = NULL;
  struct iio_channel *rx_lo_chn = NULL;

  set_param(tx_chn, tx_lo_chn, phy_dev, txcfg, TX);
  set_param(rx_chn, rx_lo_chn, phy_dev, rxcfg, RX);

 
  tx0_i = iio_device_find_channel(tx_dev, "voltage0", true);
  tx0_q = iio_device_find_channel(tx_dev, "voltage1", true);

  rx0_i = iio_device_find_channel(rx_dev, "voltage0", false);
  rx0_q = iio_device_find_channel(rx_dev, "voltage1", false);


  rxmask = iio_create_channels_mask(iio_device_get_channels_count(rx_dev));
  if (!rxmask) {
    fprintf(stderr, "Unable to alloc channels mask\n");
    shutdown();
  }

  txmask = iio_create_channels_mask(iio_device_get_channels_count(tx_dev));
  if (!txmask) {
    fprintf(stderr, "Unable to alloc channels mask\n");
    shutdown();
  }

  printf("* Enabling IIO streaming channels\n");
  
  iio_channel_enable(rx0_i, rxmask);
  iio_channel_enable(rx0_q, rxmask);
  iio_channel_enable(tx0_i, txmask);
  iio_channel_enable(tx0_q, txmask);

  printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
  
  rxbuf = iio_device_create_buffer(rx_dev, 0, rxmask);
  txbuf = iio_device_create_buffer(tx_dev, 0, txmask);

  rxstream = iio_buffer_create_stream(rxbuf, 4, pow(2, 14));
  txstream = iio_buffer_create_stream(txbuf, 4, pow(2, 14));

  size_t rx_sample_sz, tx_sample_sz;

  rx_sample_sz = iio_device_get_sample_size(rx_dev, rxmask);
  tx_sample_sz = iio_device_get_sample_size(tx_dev, txmask);

  printf("* rx_sample_sz = %l\n",rx_sample_sz);
  printf("* tx_sample_sz = %l\n",tx_sample_sz);

  const struct iio_block *txblock, *rxblock;

  
  int16_t tx_i[320] = {0};
  int16_t tx_q[320] = {0};

  read_data("rec.txt", tx_i, tx_q, 320);
 

  int16_t rx_i[1000000];
  int16_t rx_q[1000000];
  int16_t tx_file_i[1000000];
  int16_t tx_file_q[1000000];
  memset (rx_i, 0, 1000000);
  memset (rx_q, 0, 1000000);
  memset (tx_file_i, 0, 1000000);
  memset (tx_file_q, 0, 1000000);

  printf("start\n");


  int i = 0, j = 0, counter = 0;
  while (counter < 30)
    {
      
      int16_t *p_dat, *p_end;
      ptrdiff_t p_inc;
      uint32_t samples_cnt = 0;

      rxblock = iio_stream_get_next_block(rxstream);
      txblock = iio_stream_get_next_block(txstream);

      p_inc = tx_sample_sz;
      p_end = (int16_t*) iio_block_end(txblock);
      int counter_i = 0;
	      
      for (p_dat = (int16_t *)iio_block_first(txblock, tx0_i); p_dat < p_end; p_dat += p_inc / sizeof(*p_dat)){
	  if(counter_i < 320){
	    p_dat[0] = (tx_i[counter_i]) / 64.0;      
	    p_dat[1] = (tx_q[counter_i]) / 64.0;
	  } else {
	    counter_i = 0;
	  }
	  counter_i++;
	  j++ ;
	}  
       
      
      
      p_inc = rx_sample_sz;
      p_end = (int16_t*)iio_block_end(rxblock);
     
      for (p_dat = (int16_t*) iio_block_first(rxblock, rx0_i); p_dat < p_end; p_dat += p_inc / sizeof(*p_dat)) {
	rx_i[i] = p_dat[0];
	rx_q[i] = p_dat[1];
	samples_cnt++;
	i++;
      }
      printf("samples_cnt = %d\n", samples_cnt);
      printf("i = %d\n", i);


      printf("counter = %d\n", counter);
      counter++;
    }

    
  shutdown();

  write_data("rx_data.txt", rx_i, rx_q, 1000000);
  return 0;

}
  
 

