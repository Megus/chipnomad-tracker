News
====

JavaScript implementation by Alexander Kovalenko: https://github.com/alexanderk23/ayumi-js

Ayumi API reference
===================

``` c
void ayumi_configure(struct ayumi* ay, int is_ym, double clock_rate, int sr)
```

*Configures the ayumi structure.*

**ay**: The pointer to the ayumi structure.

**is_ym**: 1 if the chip is YM2149.

**clock_rate**: The clock rate of the chip.

**sr**: The output sample rate.

``` c
void ayumi_set_pan(struct ayumi* ay, int index, double pan, int is_eqp)
```

*Sets the panning value for the specified sound channel.*

**ay**: The pointer to the ayumi structure.

**index**: The index of the sound channel.

**pan**: The stereo panning value [0-1].

**is_eqp**: 1 if "equal power" panning is used.

``` c
void ayumi_set_tone(struct ayumi* ay, int index, int period)
```

*Sets the tone period value for the specified sound channel.*

**ay**: The pointer to the ayumi structure.

**index**: The index of the sound channel.

**period**: The tone period value [0-4095].

``` c
void ayumi_set_noise(struct ayumi* ay, int period)
```

*Sets the noise period value.*

**ay**: The pointer to the ayumi structure.

**period**: The noise period value [0-31].

``` c
void ayumi_set_mixer(struct ayumi* ay, int index, int t_off, int n_off, int e_on)
```

*Sets the mixer value for the specified sound channel.*

**ay**: The pointer to the ayumi structure.

**index**: The index of the sound channel.

**t_off**: 1 if the tone is off.

**n_off**: 1 if the noise is off.

**e_on**: 1 if the envelope is on.

``` c
void ayumi_set_volume(struct ayumi* ay, int index, int volume)
```

*Sets the volume for the specified sound channel.*

**ay**: The pointer to the ayumi structure.

**index**: The index of the sound channel.

**volume**: The volume [0-15].

``` c
void ayumi_set_envelope(struct ayumi* ay, int period)
```

*Sets the envelope period value.*

**ay**: The pointer to the ayumi structure.

**period**: The envelope period value [0-65535].

``` c
void ayumi_set_envelope_shape(struct ayumi* ay, int shape)
```

*Sets the envelope shape value.*

**ay**: The pointer to the ayumi structure.

**shape**: The envelope shape index [0-15].

``` c
void ayumi_process(struct ayumi* ay)
```

*Renders the next stereo sample in ay->left and ay->right.*

**ay**: The pointer to the ayumi structure.

``` c
void ayumi_remove_dc(struct ayumi* ay)
```

*Removes the DC offset from the current sample.*

**ay**: The pointer to the ayumi structure.
