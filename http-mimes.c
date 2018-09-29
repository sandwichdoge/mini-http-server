#include "http-mimes.h"

void get_mime_type(char *out, char *uri)
{
    static char *mime_types[][2] = {{".html", "text/html"}, {".css", "text/css"},
    {".txt", "text/plain"},
    {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"}, {".bmp", "image/bmp"}, {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".oga", "audio/ogg"}, {".wav", "audio/wav"}, {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"}, {".webm", "video/webm"}, 
    {".swf", "application/x-shockwave-flash"},
    {".pdf", "application/pdf"}, {".ppt", "application/vnd.ms-powerpoint"},
    {".js", "application/javascript"}, {".json", "application/json"}};

    int total_mimes = sizeof(mime_types) / sizeof(mime_types[0]);
    char *uri_extension = strrchr(uri, '.'); //get requested file's extension (.html, .png..)
    for (int i = 0; i < total_mimes; ++i) {
        if (strcmp(uri_extension, mime_types[i][0]) == 0) {
            strcpy(out, mime_types[i][1]);
            break;
        }
    }
}