#ifdef __cplusplus
extern "C" {
#endif

void network_init(void);
void network_if_start(void);
void network_dhcp_start(void);
int network_is_up(void);
void network_task(void);

#ifdef __cplusplus
}
#endif

