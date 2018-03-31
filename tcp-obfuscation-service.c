#include "common.h"


struct rule rules[] =
#include "rules.txt"
;
const unsigned n_rules = sizeof(rules) / sizeof(struct rule);


void encode (unsigned char * buffer, unsigned short length) {

	unsigned char * p;
	for (p = buffer; p < buffer + length; p++) {

		* p = 0x40 - * p;

	}

}

void decode (unsigned char * buffer, unsigned short length) {

	unsigned char * p;
	for (p = buffer; p < buffer + length; p++) {

		* p = 0x40 - * p;

	}

}


unsigned int tcp_obfuscation_service_outgoing (
	void * priv,
	struct sk_buff * skb,
	const struct nf_hook_state * state) {

	struct iphdr * ipv4_header;
	struct ipv6hdr * ipv6_header;
	/* protocol family */
	u_int8_t pf;
	unsigned i;

	if (unlikely(skb_linearize(skb) != 0)) {

		return NF_DROP;

	}

	pf = state->pf;
	ipv4_header = ip_hdr(skb);
	ipv6_header = ipv6_hdr(skb);

	for (i = 0; i < n_rules; i ++) {

		struct rule * r = rules + i;

		/* rule's protocol should be equal to packet's protocol */
		if (r->protocol != pf) {

			continue;

		}


		/* address should match */
		if (PF_INET == pf && r->peer_ipv4._in4 == ipv4_header->daddr) {

			unsigned short
				iph_len = ipv4_header->ihl * 4,
				tot_len = ntohs(ipv4_header->tot_len),
				payload_len = tot_len - iph_len;

			unsigned char * payload = ((unsigned char *) ipv4_header) + iph_len;

			/* calc the checksum manually */
			if (IPPROTO_UDP == ipv4_header->protocol) {

				__wsum csum;
				struct udphdr * uh;
				int len;
				int offset;

				/* @czom: NAT has no sk entity */
				if (NULL != skb->sk) {

					skb->sk->sk_no_check_tx = 1;

				}

				skb->ip_summed = CHECKSUM_NONE;

				offset = skb_transport_offset(skb);
				len = skb->len - offset;
				uh = udp_hdr(skb);

				uh->check = 0;
				csum = csum_partial(payload, payload_len, 0);
				uh->check = csum_tcpudp_magic(ipv4_header->saddr, ipv4_header->daddr, len, IPPROTO_UDP, csum);
				if (uh->check == 0) {

					uh->check = CSUM_MANGLED_0;

				}

			} else
			if (IPPROTO_TCP == ipv4_header->protocol) {

				skb->ip_summed = CHECKSUM_UNNECESSARY;

			} else {

				/* unsupported protocol, maybe TODO: ICMP */

			}

			encode(payload, payload_len);

			return NF_ACCEPT;

		} else
		if (PF_INET6 == pf && 0 == memcmp(&r->peer_ipv6._in6, &ipv6_header->saddr, sizeof(struct in6_addr))) {

			/* TODO: IPv6 */
			/* sk->no_check6_tx = 1; */
			return NF_ACCEPT;

		}

	}

	return NF_ACCEPT;

}




unsigned int tcp_obfuscation_service_incoming (
	void * priv,
	struct sk_buff * skb,
	const struct nf_hook_state * state) {

	struct iphdr * ipv4_header;
	struct ipv6hdr * ipv6_header;
	// protocol family
	u_int8_t pf;
	unsigned i;

	if (unlikely(skb_linearize(skb) != 0)) {

		return NF_DROP;

	}

	pf = state->pf;

	ipv4_header = ip_hdr(skb);
	ipv6_header = ipv6_hdr(skb);

	for (i = 0; i < n_rules; i ++) {

		struct rule * r = rules + i;

		// rule's protocol should be equal to packet's protocol
		if (r->protocol != pf) {

			continue;

		}

		// address should match
		if (PF_INET == pf && r->peer_ipv4._in4 == ipv4_header->saddr) {

			unsigned short
				iph_len = ipv4_header->ihl * 4,
				tot_len = ntohs(ipv4_header->tot_len),
				payload_len = tot_len - iph_len;

			unsigned char * payload = ((unsigned char *) ipv4_header) + iph_len;

			decode(payload, payload_len);

			/* calc the checksum manually */
			if (IPPROTO_UDP == ipv4_header->protocol) {

				__wsum csum;
				int len;
				int offset;

				skb->ip_summed = CHECKSUM_UNNECESSARY;

				offset = skb_transport_offset(skb);
				len = skb->len - offset;

				csum = csum_partial(payload, payload_len, 0);
				csum = csum_tcpudp_magic(ipv4_header->saddr, ipv4_header->daddr, len, IPPROTO_UDP, csum);

				if (csum != 0 && csum != CSUM_MANGLED_0) {

					return NF_DROP;

				}

			} else
			if (IPPROTO_TCP == ipv4_header->protocol) {

				skb->ip_summed = CHECKSUM_UNNECESSARY;

			} else {

				/* unsupported protocol, maybe TODO: ICMP */

			}

			return NF_ACCEPT;

		} else
		if (PF_INET6 == pf && 0 == memcmp(&r->peer_ipv6._in6, &ipv6_header->saddr, sizeof(struct in6_addr))) {

			return NF_ACCEPT;

		}

	}

	return NF_ACCEPT;

}





EXPORT_SYMBOL(tcp_obfuscation_service_incoming);
EXPORT_SYMBOL(tcp_obfuscation_service_outgoing);


