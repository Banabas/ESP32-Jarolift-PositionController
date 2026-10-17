#include <GithubReleaseOTA.h>

/**
 * @brief Construct a new Github Release OTA object
 *
 * @param owner `const char*` Github repository owner
 * @param repo `const char*` Github repository
 * @param token `const char*` Github token
 */
GithubReleaseOTA::GithubReleaseOTA(const char* owner, const char* repo, const char* token) {
    int urlSize = snprintf(NULL, 0, GITHUB_API_RELEASE_URL, owner, repo) + 1;
    this->releaseUrl = (char*)malloc(urlSize);
    if (this->releaseUrl != NULL)
        snprintf(this->releaseUrl, urlSize, GITHUB_API_RELEASE_URL, owner, repo);
    else
        ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for release URL");

    if (token != NULL) {
        this->token = (char*)malloc(strlen(token) + 1);

        if (this->token != NULL)
            strcpy(this->token, token);
        else
            ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for release Token");
    } else {
        this->token = NULL;
    }
}

/**
 * @brief Destroy the Github Release OTA object
 *
 */
GithubReleaseOTA::~GithubReleaseOTA() {
    if (this->releaseUrl != NULL) free(this->releaseUrl);

    if (this->token != NULL) free(this->token);

    if (this->ca != NULL) free(this->ca);
}

/**
 * @brief Destroy the Github Release OTA object
 *
 */
void GithubReleaseOTA::clear() { this->~GithubReleaseOTA(); }

/**
 * @brief Set website CA (Certificate Authority)
 *
 * @param ca `const char*` Certificate Authority
 */
void GithubReleaseOTA::setCa(const char* ca) {
    if (this->ca != NULL) free(this->ca);

    this->ca = (char*)malloc(strlen(ca) + 1);

    if (this->ca != NULL)
        strcpy(this->ca, ca);
    else
        ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for CA");
}

/**
 * @brief Get Latest release tag
 *
 * @return `String` Latest release tag
 */
String GithubReleaseOTA::getLatestReleaseTag() {
    GithubRelease release = getLatestRelease();
    String tag = "";
    if (release.name != NULL) tag = release.tag_name;
    freeRelease(release);
    return tag;
}

/**
 * @brief Get all release tags
 *
 * @return `std::vector<String>` List of release tags
 */
std::vector<String> GithubReleaseOTA::getReleaseTagList() {
    std::vector<String> tags;

    if (releaseUrl != NULL) {
        int code = 0;
        String payload = connectGithub(this->releaseUrl, &code);
        if (code == HTTP_CODE_OK) {
            JsonDocument releasesList;
            deserializeJson(releasesList, payload);

            JsonArray releases = releasesList.as<JsonArray>();

            for (JsonObject release : releases) tags.push_back(release["tag_name"].as<String>());

            releasesList.clear();
        }
    }

    return tags;
}

/**
 * @brief Get latest release
 *
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::getLatestRelease() {
    GithubRelease release;

    int urlSize = snprintf(NULL, 0, GITHUB_API_LATEST_RELEASE_URL, this->releaseUrl) + 1;
    char* url = (char*)malloc(urlSize);

    if (url != NULL) {
        snprintf(url, urlSize, GITHUB_API_LATEST_RELEASE_URL, this->releaseUrl);
        int code = 0;
        String payload = connectGithub(url, &code);
        free(url);

        if (code == HTTP_CODE_OK) {
            release = makeRelease(payload);
        }
    }

    return release;
}

/**
 * @brief Get release by tag name
 *
 * @param tag `const char*` Tag Name
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::getReleaseByTagName(const char* tagName) {
    GithubRelease release;

    if (tagName == NULL) {
        return release;
    }

    int urlSize = snprintf(NULL, 0, GITHUB_API_TAGS_RELEASE_URL, this->releaseUrl, tagName) + 1;
    char* url = (char*)malloc(urlSize);

    if (url != NULL) {
        snprintf(url, urlSize, GITHUB_API_TAGS_RELEASE_URL, this->releaseUrl, tagName);
        int code = 0;
        String payload = connectGithub(url, &code);
        free(url);
        if (code == HTTP_CODE_OK) release = makeRelease(payload);
    }

    return release;
}

/**
 * @brief Get asset by name
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `GithubReleaseAsset` Github Release Asset Object
 */
GithubReleaseAsset GithubReleaseOTA::getAssetByname(GithubRelease release, const char* name) {
    GithubReleaseAsset asset;

    for (GithubReleaseAsset a : release.assets) {
        if (strcmp(a.name, name) == 0) {
            return a;
        }
    }

    return asset;
}

/**
 * @brief Flash firmware
 *
 * @param asset `GithubReleaseAsset` Github Release Asset Object
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashFirmware(GithubReleaseAsset asset) { return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_FIRMWARE); }

/**
 * @brief Flash firmware
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashFirmware(GithubRelease release, const char* name) {
    GithubReleaseAsset asset = getAssetByname(release, name);
    if (asset.browser_download_url == NULL) return OTA_NULL_URL;

    return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_FIRMWARE);
}

/**
 * @brief Flash SPIFFS
 *
 * @param asset `GithubReleaseAsset` Github Release Asset Object
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashSpiffs(GithubReleaseAsset asset) { return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_SPIFFS); }

/**
 * @brief Flash SPIFFS
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashSpiffs(GithubRelease release, const char* name) {
    GithubReleaseAsset asset = getAssetByname(release, name);
    if (asset.browser_download_url == NULL) return OTA_NULL_URL;

    return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_SPIFFS);
}

/**
 * @brief Flash by asset ID
 *
 * @param assetId `int` Asset ID
 * @param flashType `int` Flash Type, `U_FLASH` or `U_SPIFFS`
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashByAssetId(int assetId, int flashType) {
    int urlSize = snprintf(NULL, 0, GITHUB_API_RELEASE_ASSETS_URL, this->releaseUrl, String(assetId).c_str()) + 1;
    char* url = (char*)malloc(urlSize);
    if (url == NULL) {
        ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for asset URL");
        return OTA_NULL_URL;
    }

    snprintf(url, urlSize, GITHUB_API_RELEASE_ASSETS_URL, this->releaseUrl, String(assetId).c_str());

    HTTPClient client;
    if (this->ca != NULL)
        client.begin(url, this->ca);
    else
        client.begin(url);

    if (token != NULL) client.setAuthorization("Bearer", token);
    client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    client.addHeader("Accept", GITHUB_API_RELEASE_ASSETS_ACCEPT_OCTET_STREAM);
    client.addHeader("X-GitHub-Api-Version", X_GITHUB_API_VERSION);

    if (client.GET() != HTTP_CODE_OK) {
        ESP_LOGE("GithubReleaseOTA", "Failed to connect to GitHub API");
        client.end();
        free(url);
        return OTA_CONNECT_ERROR;
    }

    int size = client.getSize();
    if (!Update.begin(size, flashType)) {
        ESP_LOGE("GithubReleaseOTA", "Failed to begin OTA update");
        client.end();
        free(url);
        return OTA_BEGIN_ERROR;
    }

    // Use the HTTPClient's stream directly
    Stream& stream = client.getStream();
    size_t written = 0;
    const size_t chunkSize = 1024;  // Set chunk size
    uint8_t buffer[chunkSize];
    int lastProgress = -1;

    while (written < size) {
        size_t available = stream.available();
        if (available > 0) {
            size_t readSize = stream.readBytes(buffer, min(available, chunkSize));
            if (readSize > 0) {
                if (Update.write(buffer, readSize) != readSize) {
                    ESP_LOGE("GithubReleaseOTA", "Error writing chunk");
                    client.end();
                    free(url);
                    return OTA_WRITE_ERROR;
                }
                written += readSize;
                ESP_LOGI("GithubReleaseOTA", "Written %d/%d bytes", written, size);
            }

            int progress = (written * 100) / size;
            if (progress != lastProgress) {
                if (this->progressCallback) {
                    this->progressCallback(progress);
                }
                lastProgress = progress;
            }
        }
        delay(1);
    }

    if (!Update.end()) {
        ESP_LOGE("GithubReleaseOTA", "Failed to end OTA update");
        client.end();
        free(url);
        return OTA_END_ERROR;
    }

    ESP_LOGI("GithubReleaseOTA", "OTA update successful");
    client.end();
    free(url);
    return OTA_SUCCESS;
}

/**
 * @brief Free release
 *
 * @param release `GithubRelease&` Github Release Object
 */
void GithubReleaseOTA::freeRelease(GithubRelease& release) {
    if (release.url != NULL) free((void*)release.url);
    if (release.html_url != NULL) free((void*)release.html_url);
    if (release.assets_url != NULL) free((void*)release.assets_url);
    if (release.upload_url != NULL) free((void*)release.upload_url);
    if (release.tarball_url != NULL) free((void*)release.tarball_url);
    if (release.zipball_url != NULL) free((void*)release.zipball_url);
    if (release.node_id != NULL) free((void*)release.node_id);
    if (release.tag_name != NULL) free((void*)release.tag_name);
    if (release.target_commitish != NULL) free((void*)release.target_commitish);
    if (release.name != NULL) free((void*)release.name);
    if (release.body != NULL) free((void*)release.body);
    if (release.created_at != NULL) free((void*)release.created_at);
    if (release.published_at != NULL) free((void*)release.published_at);

    for (auto& author : release.author) {
        freeAuthor(author);
    }

    for (auto& asset : release.assets) {
        freeReleaseAsset(asset);
    }
}

/**
 * @brief Free release asset
 *
 * @param asset `GithubReleaseAsset&` Github Release Asset Object
 */
void GithubReleaseOTA::freeReleaseAsset(GithubReleaseAsset& asset) {
    if (asset.url != NULL) free((void*)asset.url);
    if (asset.node_id != NULL) free((void*)asset.node_id);
    if (asset.name != NULL) free((void*)asset.name);
    if (asset.label != NULL) free((void*)asset.label);
    if (asset.content_type != NULL) free((void*)asset.content_type);
    if (asset.state != NULL) free((void*)asset.state);
    if (asset.created_at != NULL) free((void*)asset.created_at);
    if (asset.updated_at != NULL) free((void*)asset.updated_at);
    if (asset.browser_download_url != NULL) free((void*)asset.browser_download_url);
}

/**
 * @brief Free author
 *
 * @param author `GithubAuthor&` Github Author Object
 */
void GithubReleaseOTA::freeAuthor(GithubAuthor& author) {
    if (author.login != NULL) free((void*)author.login);
    if (author.node_id != NULL) free((void*)author.node_id);
    if (author.avatar_url != NULL) free((void*)author.avatar_url);
    if (author.gravatar_id != NULL) free((void*)author.gravatar_id);
    if (author.url != NULL) free((void*)author.url);
    if (author.html_url != NULL) free((void*)author.html_url);
    if (author.followers_url != NULL) free((void*)author.followers_url);
    if (author.following_url != NULL) free((void*)author.following_url);
    if (author.gists_url != NULL) free((void*)author.gists_url);
    if (author.starred_url != NULL) free((void*)author.starred_url);
    if (author.subscriptions_url != NULL) free((void*)author.subscriptions_url);
    if (author.organizations_url != NULL) free((void*)author.organizations_url);
    if (author.repos_url != NULL) free((void*)author.repos_url);
    if (author.events_url != NULL) free((void*)author.events_url);
    if (author.received_events_url != NULL) free((void*)author.received_events_url);
    if (author.type != NULL) free((void*)author.type);
}

/**
 * @brief Connect to Github Release API
 *
 * @param url `const char*` Github Repo URL
 * @param code `int` HTTP code
 * @return `String` payload
 */
String GithubReleaseOTA::connectGithub(const char* url, int* code) {
    HTTPClient http;
    if (this->ca != NULL)
        http.begin(url, this->ca);
    else
        http.begin(url);
    http.addHeader("Accept", GITHUB_API_RELEASE_ASSETS_ACCEPT_JSON);
    http.addHeader("X-GitHub-Api-Version", X_GITHUB_API_VERSION);
    if (this->token != NULL) http.setAuthorization("Bearer", this->token);

    *code = http.GET();
    String payload = http.getString();
    if (*code != HTTP_CODE_OK) {
        ESP_LOGW("GithubReleaseOTA", "GET failed: %d (%s), heap %u, largest %u", *code,
                 http.errorToString(*code).c_str(), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    }

    http.end();

    return payload;
}

/**
 * @brief Make release object
 *
 * @param payload `String` Github Release JSON payload
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::makeRelease(String payload) {
    GithubRelease githubRelease;
    JsonDocument releases;
    deserializeJson(releases, payload);

    auto copyString = [](const char* str) -> char* {
        if (str == nullptr) return nullptr;
        char* copy = (char*)malloc(strlen(str) + 1);
        if (copy != nullptr) {
            strcpy(copy, str);
        } else {
            ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for string");
        }
        return copy;
    };

    githubRelease.url = copyString(releases["url"]);
    githubRelease.html_url = copyString(releases["html_url"]);
    githubRelease.assets_url = copyString(releases["assets_url"]);
    githubRelease.upload_url = copyString(releases["upload_url"]);
    githubRelease.tarball_url = copyString(releases["tarball_url"]);
    githubRelease.zipball_url = copyString(releases["zipball_url"]);
    githubRelease.id = releases["id"];
    githubRelease.node_id = copyString(releases["node_id"]);
    githubRelease.tag_name = copyString(releases["tag_name"]);
    githubRelease.target_commitish = copyString(releases["target_commitish"]);
    githubRelease.name = copyString(releases["name"]);
    githubRelease.body = copyString(releases["body"]);
    githubRelease.draft = releases["draft"];
    githubRelease.prerelease = releases["prerelease"];
    githubRelease.created_at = copyString(releases["created_at"]);
    githubRelease.published_at = copyString(releases["published_at"]);

    JsonArray author = releases["author"].as<JsonArray>();
    for (JsonObject author : author) {
        GithubAuthor githubAuthor;

        githubAuthor.login = copyString(author["login"]);
        githubAuthor.id = author["id"];
        githubAuthor.node_id = copyString(author["node_id"]);
        githubAuthor.avatar_url = copyString(author["avatar_url"]);
        githubAuthor.gravatar_id = copyString(author["gravatar_id"]);
        githubAuthor.url = copyString(author["url"]);
        githubAuthor.html_url = copyString(author["html_url"]);
        githubAuthor.followers_url = copyString(author["followers_url"]);
        githubAuthor.following_url = copyString(author["following_url"]);
        githubAuthor.gists_url = copyString(author["gists_url"]);
        githubAuthor.starred_url = copyString(author["starred_url"]);
        githubAuthor.subscriptions_url = copyString(author["subscriptions_url"]);
        githubAuthor.organizations_url = copyString(author["organizations_url"]);
        githubAuthor.repos_url = copyString(author["repos_url"]);
        githubAuthor.events_url = copyString(author["events_url"]);
        githubAuthor.received_events_url = copyString(author["received_events_url"]);
        githubAuthor.type = copyString(author["type"]);
        githubAuthor.site_admin = author["site_admin"];

        githubRelease.author.push_back(githubAuthor);
    }

    author.clear();

    JsonArray assets = releases["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        GithubReleaseAsset githubAsset;

        githubAsset.url = copyString(asset["url"]);
        githubAsset.id = asset["id"];
        githubAsset.node_id = copyString(asset["node_id"]);
        githubAsset.name = copyString(asset["name"]);
        githubAsset.label = copyString(asset["label"]);
        githubAsset.content_type = copyString(asset["content_type"]);
        githubAsset.state = copyString(asset["state"]);
        githubAsset.size = asset["size"];
        githubAsset.download_count = asset["download_count"];
        githubAsset.created_at = copyString(asset["created_at"]);
        githubAsset.updated_at = copyString(asset["updated_at"]);
        githubAsset.browser_download_url = copyString(asset["browser_download_url"]);
        githubRelease.assets.push_back(githubAsset);
    }

    assets.clear();

    releases.clear();

    return githubRelease;
}
