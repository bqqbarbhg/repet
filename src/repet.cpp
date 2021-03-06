#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/*
const char *test_string = "comet compression compressor compressor"
	"test.computer color comp ressor compressor";
*/
/*
const char *test_string = "<title>title</title>";
*/
// const char *test_string = "Batman&Robin Batman Batman Bat Bat Batman&Rob";
//const char *test_string = "test test test";

#define HASH_TABLE_SIZE 1024

unsigned hash3(const char *data) {
	unsigned char *udata = (unsigned char*)data;
	return (udata[0] + udata[1] * 23 + udata[2] * 67) % HASH_TABLE_SIZE;
}

void print_highlight(const char *data, int size, int pos, int length, int context, int pre_len)
{
	context = 100;

	int left_margin = pos - context;
	if (left_margin < 0) left_margin = 0;
	int right_margin = pos + length + context;
	if (right_margin >= size) right_margin = size;

	for (int i = left_margin; i < right_margin; i++) {
		putchar(data[i]);
	}
	putchar('\n');
	for (int i = 0; i < pre_len; i++) {
		putchar(' ');
	}
	for (int i = left_margin; i < right_margin; i++) {
		putchar(i >= pos && i < pos + length ? '~' : ' ');
	}
	putchar('\n');
}

#define DO_LOG 0

#if 0
#define print_highlight(data, size, pos, length, context, pre_len)
#endif

void test_compress(const char *data, size_t length)
{
	size_t skip_table_size = length;
	if (skip_table_size > 0x10000) {
		skip_table_size = 0x10000;
	}

	uint16_t *skip_table = (uint16_t*)malloc(skip_table_size * sizeof(uint16_t));
	memset(skip_table, 0xFF, skip_table_size * sizeof(uint16_t));
	uint16_t *hash_table = (uint16_t*)malloc(HASH_TABLE_SIZE * sizeof(uint16_t));
	memset(hash_table, 0xFF, HASH_TABLE_SIZE * sizeof(uint16_t));

	int scans = 0;
	int skips = 0;
	int hashes = 0;

	unsigned last_match_pos = 0;
	unsigned last_match_len = 0;
	unsigned last_end_hash = 0;

	const char *base = data;
	size_t base_length = length;

	unsigned int min_forget_dist = 16000;
	unsigned forget_length = 0x10000 - min_forget_dist;
	
	for (;;) {

		unsigned block_length = forget_length;
		if (base_length < block_length) {
			block_length = (unsigned)base_length;
		}
		for (unsigned pos = 0; pos < block_length; pos++) {
			if (base_length - pos < 3) {
				// No full trigraph left, not worth finding a match.
				continue;
			}

			const char *cur_str = &base[pos];
			unsigned max_match_len = (unsigned)(base_length - pos);

			// Get the linked list of the previous occourences of the trigraph at the
			// current position.
			unsigned hash = hash3(&base[pos]);
			uint16_t prev = hash_table[hash]; hashes++;
			hash_table[hash] = (uint16_t)pos;

#if DO_LOG
			putchar('\n');
			print_highlight(base, base_length, pos, 1, 30, printf("|> "));
#endif
			int t = 0;

			if (prev == 0xFFFF) {
#if DO_LOG
				printf("No previous match for [%.3s]\n", &base[pos]);
#endif

				// The trigraph hasn't been seen yet, so there can be no match.
				// Initialize linked list with end node and continue scanning.
				skip_table[pos] = 0xFFFF;
				last_match_len = 0;
				continue;
			}

			// Link to the chain.
			skip_table[pos] = (uint16_t)(pos - prev);

			unsigned end_dist = 0;
			unsigned end_hash = 0;
			unsigned end_pos = 0;
			int dbg_hash_pos;
			unsigned target_len = 3;
			unsigned best_pos = 0;
			unsigned best_len = 0;

			uint16_t check_pos = prev;

			if (last_match_len > 3 && base_length - pos > 3) {

				int len = last_match_len - 1;

				end_hash = last_end_hash;
				if (end_hash == HASH_TABLE_SIZE) {

					last_match_pos++;
					last_match_len--;

#if DO_LOG
					printf("Last match proven optimal\n");
#endif

					continue;
				}
				end_pos = hash_table[end_hash];
				if (end_pos == 0xFFFF) {

					last_match_pos++;
					last_match_len--;

#if DO_LOG
					printf("Last match proven optimal\n");
#endif

					continue;
				}

				end_dist = len - 2;
				target_len = len + 1;

				best_pos = last_match_pos + 1;
				best_len = len;

#if DO_LOG
				printf("Searching with [%.3s]..%d..[%.3s]\n",
					&base[pos], end_dist, &base[pos + last_match_len - 3]);
#endif

				// Synchronize the begin and end trigraphs so that they are separated
				// by the searched for amount of bytes.
				for (;;) {
					int diff = end_pos - check_pos - end_dist;
					if (diff == 0)
						break;
					else if (diff > 0) {
						uint16_t skip = skip_table[end_pos]; skips++;
						if (skip > end_pos) break;
						end_pos -= skip;
					} else {
						uint16_t skip = skip_table[check_pos]; skips++;
						if (skip > check_pos) break;
						check_pos -= skip;
					}
				}
			} else {
#if DO_LOG
				printf("Searching with [%.3s]\n", &base[pos]);
#endif
			}

			for (;;)
			{
				// Calculate the maximum bound for the match.
				unsigned check_len = (unsigned)(pos - check_pos);
				unsigned max_check_len = check_len < max_match_len
					? check_len : max_match_len;

				scans++;

				// Match as far as possible.
				const char *check_str = &base[check_pos];
				unsigned len;
				for (len = 0; len < max_check_len; len++) {
					if (cur_str[len] != check_str[len])
						break;
				}

				if (len >= target_len) {
#if DO_LOG
					print_highlight(base, base_length, pos, len, 10, printf("%d: ", t));
					print_highlight(base, base_length, check_pos, len, 10, printf("%d: ", t));
#endif
					t++;

					best_len = len;
					best_pos = check_pos;

					if (pos + len >= base_length - 1) {
						// We have found the longest possible match, nothing to search for.
						end_hash = HASH_TABLE_SIZE;
						break;
					}

					// We found a new longest match, update end trigraph accordingly.
					end_hash = hash3(&base[pos + len - 2]);
					end_pos = hash_table[end_hash]; hashes++;
					end_dist = len - 2;
					target_len = len + 1;

					if (end_pos == 0xFFFF) {
						// The buffer does not contain any occourences of the end trigraph,
						// so we have found the longest match there is.
						break;
					}

					dbg_hash_pos = (int)(pos + len - 2);

#if DO_LOG
					printf("Searching with [%.3s]..%d..[%.3s]\n",
						&base[pos], end_dist, &base[dbg_hash_pos]);
#endif

				} else {
					// In this case we hit a false match candidate. Carry on.
				}

				{
					// Move to the next match candidate.
					uint16_t skip = skip_table[check_pos]; skips++;
					if (skip > check_pos) break;
					check_pos -= skip;
				}

				if (best_len > 0) {
					// Synchronize the begin and end trigraphs so that they are separated
					// by the searched for amount of bytes.
					for (;;) {
						int diff = end_pos - check_pos - end_dist;
						if (diff == 0)
							break;
						else if (diff > 0) {
							uint16_t skip = skip_table[end_pos]; skips++;
							if (skip > end_pos) break;
							end_pos -= skip;
						} else {
							uint16_t skip = skip_table[check_pos]; skips++;
							if (skip > check_pos) break;
							check_pos -= skip;
						}
					}

					// Could not find a potential match anymore.
					if ((unsigned)(end_pos - check_pos) != end_dist)
						break;
				}
			}

			last_match_pos = best_pos;
			last_match_len = best_len;
			last_end_hash = end_hash;
		}

		base_length -= block_length;
		if (base_length == 0)
			break;

		// Need to rebase
		for (unsigned i = 0; i < HASH_TABLE_SIZE; i++) {
			if (hash_table[i] < block_length) {
				// Fell off the range of the compressor, "forget" the pointer.
				hash_table[i] = 0xFFFF;
			} else {
				// Still valid, adjust base.
				hash_table[i] -= (uint16_t)block_length;
			}
		}
		base += block_length;
	}

	free(skip_table);
	free(hash_table);

	printf("Hashes: %d\n", hashes);
	printf("Skips: %d\n", skips);
	printf("Scans: %d\n", scans);
}

int main(int argc, char** argv)
{
	FILE *fl = fopen("testdata.txt", "r");
	char *data = (char*)malloc(1024*1024);
	size_t len = fread(data, 1, 1024*1024, fl);

	test_compress(data, len);
	fclose(fl);
	
	getchar();
}
