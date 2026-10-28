#ifndef DN_MUSIC_H
#define DN_MUSIC_H
void S_MusicInit(void);
void S_MusicShutdown(void);
void S_MusicPlay(const char *name, qboolean looping);
void S_MusicStop(void);
void S_MusicUpdate(void);
void S_MusicActivate(qboolean active);
#endif
