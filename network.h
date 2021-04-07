#ifdef __cplusplus
extern "C" {
#endif

void network_init(void);
void network_if_start(void);
int network_is_up(void);
void network_task(void);

extern struct netif netif_eth0;

#ifdef __cplusplus
}
#endif

