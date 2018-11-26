#include "http-mimes.h"

//For full list of MIME types, visit: https://www.freeformatter.com/mime-types-list.html

void get_mime_type(char *out, char *uri)
{
    static char *mime_types[][2] = {{".html", "text/html"}, {".css", "text/css"},
    {".htm", "text/html"}, {".xhtml", "application/xhtml+xml"},
    {".php", "text/html"},
    {".txt", "text/plain"}, {".py", "text/html"}, {".cgi", "text/html"},
    {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"}, {".bmp", "image/bmp"}, {".gif", "image/gif"},
    {".ico", "image/x-icon"}, {".svg", "image/svg+xml"},
    {".oga", "audio/ogg"}, {".wav", "audio/wav"}, {".mp3", "audio/mpeg"},
    {".midi", "audio/midi"}, {".mpg", "audio/mpeg"}, {".aac", "audio/aac"},
    {".mp4", "video/mp4"}, {".webm", "video/webm"}, 
    {".mpeg", "video/mpeg"}, {".3gp", "video/3gpp"},
    {".xml", "application/xml"},
    {".swf", "application/x-shockwave-flash"},
    {".ttf", "application/octet-stream"}, {".otf", "font/otf"},
    {".pdf", "application/pdf"}, {".ppt", "application/vnd.ms-powerpoint"},
    {".js", "application/javascript"}, {".json", "application/json"}};
    int total_mimes = sizeof(mime_types) / sizeof(mime_types[0]);

    strcpy(out, "text/plain"); //default MIME if no other types are detected.

    char *uri_extension = strrchr(uri, '.'); //get requested file's extension (.html, .png..)
    for (int i = 0; i < total_mimes; ++i) {
        if (strcmp(uri_extension, mime_types[i][0]) == 0) {
            strcpy(out, mime_types[i][1]);
            break;
        }
    }
}