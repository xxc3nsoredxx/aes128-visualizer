#ifndef OPS_H_20200520_200225
#define OPS_H_20200520_200225

void str_bytes (char *dest, const char *src, unsigned int len);
void key_expand (const char *key);
void add_round_key (unsigned int round);
char sub_byte (char byte);
void shift_row (char *row, unsigned int amt);
void mix_col (unsigned int col);

#endif /* OPS_H_20200520_200225 */
