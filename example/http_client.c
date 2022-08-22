
#include <arpa/inet.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ghttp.h"

#define TEST_URL "http://eu.httpbin.org/"

// telling the server to look for the file for 3 seconds before giving up
#define MAX_TIMEOUT "3000" // 3 seconds request-timeout

static char *USER_AGENT = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/103.0.0.0 Safari/537.36";

#define HTTP_LOG(_fmt, ...)  fprintf(stderr, "[%s@%d" _fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define HTTP_LOGD(_fmt, ...) HTTP_LOG("/D] " _fmt, ##__VA_ARGS__)
#define HTTP_LOGI(_fmt, ...) HTTP_LOG("/I] " _fmt, ##__VA_ARGS__)
#define HTTP_LOGW(_fmt, ...) HTTP_LOG("/W] " _fmt, ##__VA_ARGS__)
#define HTTP_LOGE(_fmt, ...) HTTP_LOG("/E] " _fmt, ##__VA_ARGS__)

// create a fixed length of random string
static char *http_random_string(int len)
{
    int i = 0, val = 0;
    const char *base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int base_len = (int)strlen(base);

    char *random_str = (char *)malloc(sizeof(char) * (len + 1));
    srand((unsigned int)time(NULL));

    for (; i < len; i++) {
        val = 1 + (int)((float)(base_len - 1) * rand() / (RAND_MAX + 1.0));
        random_str[i] = base[val];
    }

    random_str[len] = 0;
    return random_str;
}

/*
 * form body concatenation function
 *
 * @param dst     a buffer used to make concatenation of strings
 * @param src     a buffer to be concated to the dst
 * @param src_len src buffer length
 *
 * @return the end pointer of the dst, used for the next concatenation
 * */
char *http_memconcat(char *dst, const char *src, size_t src_len)
{
    memcpy(dst, src, src_len);
    char *p_end = dst + src_len;
    return p_end;
}

static int http_get(char *uri, char *params, char *result, int *result_len)
{
    int ret = -1;
    ghttp_request *request = ghttp_request_new();
    if (!request) {
        return -1;
    }

    if (params != NULL && strlen(params) > 0) {
        char tmp[1024] = {0};
        strcpy(tmp, uri);
        if (strchr(tmp, '?') == NULL) {
            strcat(tmp, "?");
        }
        strcat(tmp, params);
        HTTP_LOGD("http uri: %s", tmp);
        if (ghttp_set_uri(request, tmp) == -1)
            goto ec;
    } else {
        if (ghttp_set_uri(request, uri) == -1)
            goto ec;
    }

    ghttp_set_type(request, ghttp_type_get);
    ghttp_set_header(request, "User-Agent", USER_AGENT);

    ghttp_set_header(request, "Connection", "close");

    // ghttp_set_header(request, http_hdr_Timeout, MAX_TIMEOUT);

    ghttp_set_header(request, http_hdr_Accept, "application/json");

    ghttp_prepare(request);

    ghttp_status status = ghttp_process(request);
    if (status == ghttp_error)
        goto ec;

    int http_code = ghttp_status_code(request);
    HTTP_LOGD("http code: %d", http_code);
    if (http_code <= 0)
        goto ec;

    char *body = ghttp_get_body(request);
    *result_len = ghttp_get_body_len(request);
    memcpy(result, body, *result_len);

    ret = 0;

    if (0) {

        // get remote ip and port
        char *remote_ip;
        int remote_port;

        int socket = ghttp_get_socket(request);

        struct sockaddr_storage addr;
        socklen_t socket_len = sizeof(addr);
        getpeername(socket, (struct sockaddr *)&addr, &socket_len);
        if (addr.ss_family == AF_INET) {
            // ipv4
            struct sockaddr_in *s = (struct sockaddr_in *)&addr;
            remote_port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, remote_ip, sizeof(remote_ip));
        } else {
            // ipv6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
            remote_port = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, remote_ip, sizeof(remote_ip));
        }
    }

ec:
    ghttp_request_destroy(request);
    return ret;
}

int http_post(char *uri, char *params, char *result, int *result_len)
{
    char szVal[1024];
    ghttp_status status;
    printf("%s\n", params); // test
    ghttp_request *request = ghttp_request_new();

    if (ghttp_set_uri(request, uri) == -1)
        return -1;

    if (ghttp_set_type(request, ghttp_type_post) == -1) // post
        return -1;

    ghttp_set_header(request, "User-Agent", USER_AGENT);
    ghttp_set_header(request, "Accept", "application/json");
    ghttp_set_header(request, "Content-Type", "text/plain; charset=utf-8");
    ghttp_set_header(request, http_hdr_Timeout, MAX_TIMEOUT);

    // ghttp_set_header(request, "Authorization", auth_header);

    // ghttp_set_sync(request, ghttp_sync); //set sync

    if (params != NULL) {
        int len = strlen(params);
        if (len > 0)
            ghttp_set_body(request, params, len); //
    }
    ghttp_prepare(request);
    status = ghttp_process(request);
    if (status == ghttp_error)
        return -1;

    if (result) {
        char *body = ghttp_get_body(request);
        strcpy(result, body);
    }

    if (result_len) {
        *result_len = ghttp_get_body_len(request);
    }

    ghttp_request_destroy(request);
    return 0;
}

static int http_download_progress(long bytes_read, long bytes_total)
{
    static int report_percent = 0;

    if (bytes_total > 0) {
        if (bytes_read * 100 / bytes_total >= report_percent + 2) {
            report_percent = bytes_read * 100 / bytes_total;
            HTTP_LOGD("total %ld, read %ld, %02d%% completed.", bytes_total, bytes_read,
                      report_percent);
        }
    }

    return 0;
}

static int http_download(char *_pUrl, char *_pPath, int (*on_progress)(long progress, long total))
{
    const int l_chunk_size = 128 * 1024;
    int n = 0;
    int ret = -1;
    ghttp_status status;
    ghttp_current_status current_status;
    int ReturnCode = 0;
    int current_body_len = 0;
    int current_file_wirte = 0;
    int bytes_save = 0;
    int FileSize;
    char *RespBody = NULL;
    FILE *fptr;
    int report_percent = 0;

    if (!_pPath || !_pUrl || strlen(_pUrl) == 0) {
        HTTP_LOGE("Download args error.");
        return ret;
    }

    ghttp_request *request = ghttp_request_new();
    if (!request) {
        HTTP_LOGE("Could not create an http request !");
        return ret;
    }

    if (ghttp_set_uri(request, _pUrl) == ghttp_error)
        goto ec;

    HTTP_LOGD("Download Url: %s", _pUrl);
    if (ghttp_set_type(request, ghttp_type_get) == ghttp_error)
        goto ec;

    ghttp_set_header(request, "Connection", "close");

    ghttp_set_header(request, http_hdr_Timeout, MAX_TIMEOUT);

    // call async
    if (ghttp_set_sync(request, ghttp_async) == ghttp_error)
        goto ec;

    ghttp_set_chunksize(request, l_chunk_size);

    /* connect */
    ghttp_prepare(request);
    HTTP_LOGD("Download start.");
    /* receive */
    do {
        status = ghttp_process(request);
        if (status == ghttp_error) {
            goto ec;
        }
        current_status = ghttp_get_status(request);

        switch (current_status.proc) {
        case ghttp_proc_request:
            HTTP_LOGD("Download request send.");
            break;
        case ghttp_proc_response_hdrs:
            HTTP_LOGD("Download request responsed.");
            break;
        case ghttp_proc_response:
            /* check if return code error */
            if (!ReturnCode) {
                ReturnCode = ghttp_status_code(request);
                HTTP_LOGD("Download return code: %d", ReturnCode);
                if (ReturnCode >= 400 || ReturnCode < 200) {
                    goto ec;
                }

                if (ReturnCode == 302) {
                    HTTP_LOGE("FIXME handle 302 here");
                    goto ec;
                }

                fptr = fopen(_pPath, "wb");
                if (fptr == NULL) {
                    HTTP_LOGE("Error to open file!");
                    goto ec;
                }
            }
            if ((current_status.bytes_read - bytes_save) >= l_chunk_size) {
                /* flush buf to resp body and save resp body to file */
                ghttp_flush_response_buffer(request);
                current_body_len = ghttp_get_body_len(request);
                if (current_body_len) {
                    RespBody = ghttp_get_body(request);
                    if (!RespBody) {
                        HTTP_LOGE("Get resp body error!");
                        goto fc;
                    }
                    current_file_wirte = fwrite(RespBody, 1, current_body_len, fptr);
                    if (current_file_wirte != current_body_len) {
                        HTTP_LOGE("File write error!");
                        goto fc;
                    }
                    bytes_save += current_body_len;
                }
            }

            /* print progress */
            if (current_status.bytes_total > 0 && on_progress) {
                on_progress(current_status.bytes_read, current_status.bytes_total);
            }
            // usleep(100*1000);
            break;
        case ghttp_proc_none:
            break;
        default:
            goto ec;
        }
    } while (status != ghttp_done);

    /* get buffer remain */
    current_body_len = ghttp_get_body_len(request);
    RespBody = ghttp_get_body(request);
    if (current_body_len && RespBody) {
        current_file_wirte = fwrite(RespBody, 1, current_body_len, fptr);
        if (current_file_wirte != current_body_len) {
            HTTP_LOGE("File write error!");
            goto fc;
        }
        bytes_save += current_body_len;
    }

    if (on_progress) {
        on_progress(bytes_save, bytes_save);
    }

    HTTP_LOGD("File write %d bytes.", bytes_save);
    ret = 0;

fc:
    fclose(fptr);
ec:
    ghttp_request_destroy(request);
    return ret;
}

int main(int argc, const char *argv[])
{
    int ret = 0;
    char body[1024 * 8] = {0};
    int body_size = 0;

    // char *get_uri = "http://eu.httpbin.org/get";
    char *get_uri =
        "http://x.dmsys.com/"
        "GetXml?customCode=A999(M.03)&versionCode=1.0.00.1&mac=&language=zh&time=120&versionflag=1";
    ret = http_get(get_uri, NULL, body, &body_size);
    if (ret == 0) {
        // parse json body
        printf("body %s, %d\n", body, body_size);
    }

    char *w_uri = "http://www.yuanxiapi.cn/api/theweather/";
    char *param = "city=武汉市";
    ret = http_get(w_uri, param, body, &body_size);
    if (ret == 0) {
        // parse json body
        printf("body %s, %d\n", body, body_size);
    }

    char *post_uri = "http://httpbin.org/post";
    http_post(post_uri, "123abc", body, &body_size);
    printf("body %s, %d\n", body, body_size);

    // char *uri = "http://kodo.router7.com/test.zip";
    // char *path = "/tmp/test.zip";

    char *uri = "http://files.dmsys.com/OTA/FW/A999(M.03)_1.0.01.5.bin";
    char *path = "A999.bin";

    http_download(uri, path, http_download_progress);
}
