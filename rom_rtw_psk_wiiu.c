#include <stddef.h>
#include <stdint.h>

extern void rt_hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *pmk, size_t pmk_len, uint8_t *output);
extern int wpa_key_mgmt_sha256(int akm);
extern int wpa_key_mgmt_sae(int akm);
extern int sha256_prf(const uint8_t *key, size_t key_len, const char *label,
		const uint8_t *data, size_t data_len, uint8_t *buf, size_t buf_len);
extern void *_memcpy(void *dst, const void *src, size_t num);
extern int _memcmp(const void *ptr1, const void *ptr2, size_t num);

#define ETH_ALEN 6
#define WPA_NONCE_LEN 32

#define os_memcpy _memcpy
#define os_memcmp _memcmp

void rom_psk_CalcPTK(int akmp, uint8_t *addr1, uint8_t *addr2, uint8_t *nonce1, uint8_t *nonce2,
                    const uint8_t *pmk, size_t pmk_len, void *ptk, size_t ptk_len)

{
  uint8_t data[2 * ETH_ALEN + 2 * WPA_NONCE_LEN];
  uint8_t tmp[80];

  // Copy addr1 and addr2 depending on whichever is smallest
  if (os_memcmp(addr1, addr2, ETH_ALEN) < 0) {
    os_memcpy(data, addr1, ETH_ALEN);
    os_memcpy(data + ETH_ALEN, addr2, ETH_ALEN);
  } else {
    os_memcpy(data, addr2, ETH_ALEN);
    os_memcpy(data + ETH_ALEN, addr1, ETH_ALEN);
  }

  if (os_memcmp(nonce1, nonce2, WPA_NONCE_LEN) < 0) {
	os_memcpy(data + 2 * ETH_ALEN, nonce1, WPA_NONCE_LEN);
	os_memcpy(data + 2 * ETH_ALEN + WPA_NONCE_LEN, nonce2, WPA_NONCE_LEN);
  } else {
    os_memcpy(data + 2 * ETH_ALEN, nonce2, WPA_NONCE_LEN);
	os_memcpy(data + 2 * ETH_ALEN + WPA_NONCE_LEN, nonce1, WPA_NONCE_LEN);
  }

  if (wpa_key_mgmt_sha256(akmp) == 0 && wpa_key_mgmt_sae(akmp) == 0) {
    uint8_t key[100];
    os_memcpy(key, "Pairwise key expansion", 0x16);
    key[0x16] = 0;
    os_memcpy(&key[0x17],data,sizeof(data));
    for (int i = 0; i < 4; i++) {
        key[sizeof(key)-1] = i;
        rt_hmac_sha1(key,100,pmk,pmk_len,&tmp[20*i]);
    }
  } else {
    sha256_prf(pmk, pmk_len, "Pairwise key expansion", data, sizeof(data), tmp, ptk_len);
  }

  int rot_len = 48;
  uint8_t rotate_data[3];
  os_memcpy(rotate_data, tmp, 3);
  os_memcpy(tmp, tmp + 3, rot_len - 3);
  os_memcpy(tmp + rot_len - 3, rotate_data, 3);

  os_memcpy(ptk, tmp, 0x40);
}