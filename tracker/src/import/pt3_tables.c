#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <corelib/corelib_file.h>

static uint16_t PT3ToneTable_0[96];
static uint16_t PT3ToneTable_1[96];
static uint16_t PT3ToneTable_2[96];
static uint16_t PT3ToneTable_3[96];

const uint16_t* PT3ToneTables[4] = {
	PT3ToneTable_0, PT3ToneTable_1, PT3ToneTable_2, PT3ToneTable_3
};

const char* PT3TableNames[4] = {
	"PT3 Table 0",
	"PT3 Table 1",
	"PT3 Table 2",
	"PT3 Table 3"
};

static int loadPT3TableFromCSV(const char* path, uint16_t* table, int maxEntries) {
	int fileId = fileOpen(path, 0);
	if (fileId == -1) return 0;

	char* line;
	int entryCount = 0;
	int headerSkipped = 0;

	while ((line = fileReadString(fileId)) != NULL && entryCount < maxEntries) {
		if (!headerSkipped) {
			headerSkipped = 1;
			continue;
		}

		char* comma = strchr(line, ',');
		if (comma) {
			char* periodStr = comma + 1;
			int period = atoi(periodStr);
			if (period > 0) {
				table[entryCount++] = (uint16_t)period;
			}
		}
	}

	fileClose(fileId);
	return entryCount;
}

void loadPT3TablesFromCSV() {
	static int loaded = 0;
	if (loaded) return;

	const char* tableFiles[4] = {"PT3-0.csv", "PT3-1.csv", "PT3-2.csv", "PT3-3.csv"};
	uint16_t* tables[4] = {PT3ToneTable_0, PT3ToneTable_1, PT3ToneTable_2, PT3ToneTable_3};

	const char* possiblePaths[] = {
		"../../packaging/common/pitch-tables/%s",
		"./packaging/common/pitch-tables/%s",
		"../packaging/common/pitch-tables/%s"
	};

	for (int i = 0; i < 4; i++) {
		int tableLoaded = 0;
		for (int p = 0; p < 3 && !tableLoaded; p++) {
			char path[256];
			snprintf(path, sizeof(path), possiblePaths[p], tableFiles[i]);

			int entriesLoaded = loadPT3TableFromCSV(path, tables[i], 96);
			if (entriesLoaded == 96) {
				tableLoaded = 1;
			}
		}
	}

	loaded = 1;
}
