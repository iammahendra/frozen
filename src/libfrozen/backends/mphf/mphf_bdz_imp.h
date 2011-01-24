#ifndef MPHF_BDZ_IMP_H
#define MPHF_BDZ_IMP_H

ssize_t  mphf_bdz_imp_new      (mphf_t *mphf, uint64_t nelements, uint32_t value_bits);
ssize_t  mphf_bdz_imp_insert   (mphf_t *mphf, char *key, size_t key_len, uint64_t  value);
ssize_t  mphf_bdz_imp_query    (mphf_t *mphf, char *key, size_t key_len, uint64_t *value);
uint32_t mphf_bdz_imp_nstores  (uint64_t nelements, uint32_t value_bits);

#endif