# goldsrc_ghost

Currently, this is just a prototype. This is a .dem preprocessor that extracts position and time from demo files. The generated file size is significantly smaller than a full demo, and the retrieval of position from a given time is efficient.

# todo

- A demo file generated with bxt can contain playback from different maps. Map info is relevant to render only when ghost and runner are in the same map. This should be done by splitting the ghost entries when a map change is detected.
- Add a few more convenient info, like viewangles, and some animations, so that a proper player can be rendered just as multiplayer.
