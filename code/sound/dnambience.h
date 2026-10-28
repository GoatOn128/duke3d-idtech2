/* Listener-local native map ambience; no authoritative gameplay or RNG changes. */
#ifndef DN_AMBIENCE_H
#define DN_AMBIENCE_H

void S_UpdateAmbience(const float *origin);
void S_ResetAmbience(void);
void S_AmbientLoop(int marker, const char *name, float gain);

#endif
