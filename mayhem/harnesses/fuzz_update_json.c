/* Fuzz Catime's update-check response handling (network input from the GitHub releases API):
 * ParseGitHubRelease() extracts tag_name / a safe download URL / body (with JSON unescaping),
 * then the app compares the tag to its own version with CompareVersions(). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "update/update_internal.h"
#include "update_checker.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    char* json = (char*)malloc(size + 1);
    if (!json) return 0;
    memcpy(json, data, size);
    json[size] = '\0';
    char version[64], url[512], notes[4096];
    if (ParseGitHubRelease(json, version, sizeof version, url, sizeof url, notes, sizeof notes)) {
        (void)CompareVersions(version, "2.0.0");
        (void)CompareVersions("1.4.0-beta.2", version);
    }
    free(json);
    return 0;
}
