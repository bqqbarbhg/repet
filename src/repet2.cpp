
#define SEARCH_CHAIN_COUNT 2048
#define SEARCH_DATA_COUNT 4096
#define SEARCH_TUPLE_SIZE 3

#define SEARCH_MAX_DIST 32000
#define SEARCH_MAX_LENGTH 258

struct Search_Chain
{
	int base;
	uint16_t size, capacity;
	uint32_t data;
};

struct Search_Context
{
	Chain *chain_table;
	uint16_t *chain_data;
};

Search_Context init_search_context()
{
	Search_Context context;

	return context;
}

void test_compress(const char *data, int length)
{
	Chain *chains = (Search_Chain*)malloc(SEARCH_CHAIN_COUNT * sizeof(Search_Chain));
	uint16_t *chain_data = (uint16_t*)malloc(SEARCH_DATA_COUNT * sizeof(uint16_t));

	for (unsigned i = 0; i < SEARCH_CHAIN_COUNT; i++) {
		chains[i].base = -SEARCH_MAX_DIST;
	}

	for (int pos = 0; pos < length; pos++) {
		if (pos + SEARCH_TUPLE_SIZE < length) {
			const char *pos_str = &data[pos];

			unsigned hash = tuplehash(pos_str) % SEARCH_CHAIN_COUNT;
			Chain *chain = &chains[hash];
			
			if (pos - chain->base < SEARCH_MAX_DIST) {
				// Never seen or too far to reference: Reset the base and orphan the
				// possible chain.
				chain->base = pos;
				chain->size = 0;
				chain->capacity = 0;
				chain->data = 0;

				continue;
			}

			int prev = chain->base;
			const char *prev_str = &data[pos];

			// Compare the reference byte-by-byte.
			int length;
			for (length = 0; length < SEARCH_MAX_LENGTH; length {
				if (pos_str[i] != prev_str[i]) {
					break;
				}
			}
		}
	}

	free(chains);
	free(chain_data);
}

