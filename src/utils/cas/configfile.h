#ifndef CAS_CONFIGFILE_H
#define CAS_CONFIGFILE_H

#define IMG_TEXTLINES 6

typedef struct {
        char font[120];
        char source[120];
        int x[6];
        int y[6];
        int enabled[6];
        float size;
} CONF;

int writeconfig(void);
int readconfig(CONF *config);

#endif
