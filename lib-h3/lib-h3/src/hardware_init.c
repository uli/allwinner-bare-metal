#include <h3.h>

int32_t hardware_get_mac_address(/*@out@*/uint8_t *mac_address) {
	const uint32_t mac_lo = H3_EMAC->ADDR[0].LOW;
	const uint32_t mac_hi = H3_EMAC->ADDR[0].HIGH;

#ifndef NDEBUG
	printf ("H3_EMAC->ADDR[0].LOW=%08x, H3_EMAC->ADDR[0].HIGH=%08x\n", mac_lo, mac_hi);
#endif

	mac_address[0] = (mac_lo >> 0) & 0xff;
	mac_address[1] = (mac_lo >> 8) & 0xff;
	mac_address[2] = (mac_lo >> 16) & 0xff;
	mac_address[3] = (mac_lo >> 24) & 0xff;
	mac_address[4] = (mac_hi >> 0) & 0xff;
	mac_address[5] = (mac_hi >> 8) & 0xff;

#ifndef NDEBUG
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
#endif

	return 0;
}
