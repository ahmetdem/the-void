#include "linux/ip.h"
#include "linux/module.h"
#include "linux/netfilter.h"
#include "linux/netfilter_ipv4.h"
#include "linux/printk.h"
#include "linux/seq_file_net.h"
#include "void_common.h"

MODULE_AUTHOR("THE GUARDIAN");
MODULE_DESCRIPTION("THE GUARDIAN GUARDS FROM SHADOWS");
MODULE_LICENSE("GPL");

static unsigned int hook_func(void *priv, struct sk_buff *skb,
                              const struct nf_hook_state *state) {
  struct iphdr *ipheader = ip_hdr(skb);

  if (is_ip_banned(ipheader->saddr)) {
    pr_info("Shield: Blocked packet from banned IP!\n");
    return NF_DROP;
  }

  return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops = {
    .hook = hook_func,
    .pf = NFPROTO_IPV4,
    .hooknum = NF_INET_LOCAL_IN,
    .priority = NF_IP_PRI_FIRST,
};

static int __init init_fn(void) {
  pr_info("Hello the guardian.\n");

  int err = nf_register_net_hook(&init_net, &hook_ops);
  if (err < 0)
    return err;

  return 0;
}

static void __exit exit_fn(void) {
  pr_info("Goodbye the guardian.\n");
  nf_unregister_net_hook(&init_net, &hook_ops);
}

module_init(init_fn);
module_exit(exit_fn);
