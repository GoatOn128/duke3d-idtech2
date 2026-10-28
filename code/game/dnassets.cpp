/* Bounded reads of game assets from the active game directory. */
#include "dnlocal.h"
#include "dncon.h"

void *DN_ReadAsset(const char *relative, size_t *length)
{
    char path[1024];
    FILE *f;
    long size;
    char *data;
    const char *base = gi.cvar("basedir", ".", 0)->string;
    const char *game = gi.cvar("game", "", 0)->string;
    if (strstr(relative, "..") || relative[0] == '/' || strchr(relative, ':'))
    {
        return NULL;
    }
    snprintf(path, sizeof(path), "%s/%s/%s", base, *game ? game : "basedn", relative);
    f = fopen(path, "rb");
    if (!f)
    {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || size > 32 * 1024 * 1024)
    {
        fclose(f);
        return NULL;
    }
    rewind(f);
    data = (char *)(malloc((size_t)size + 1));
    if (!data)
    {
        fclose(f);
        return NULL;
    }
    if (fread(data, 1, size, f) != (size_t)size)
    {
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);
    data[size] = 0;
    if (length)
    {
        *length = size;
    }
    return data;
}
