@rem +-------+---+---+---+---+---------------------------------------------+
@rem |\ feat.| i | c | t | n | s                                           |
@rem | \     | m | o | e | o | u +-----------------------------------------+
@rem |  \    | p | l | x | r | b |                                         |
@rem |   \   | o | o | t | m | o |                                         |
@rem |    \  | r | r | u | a | b |                                         |
@rem |     \ | t | s | r | l | j |                                         |
@rem | type \|   |   | e | s | s | Program                                 |
@rem +-------+---+---+---+---+---+-----------------------------------------+

@rem | .3mf  | x | x |   |   |   | 3D Metafile                             |
call make_plugin imp_3dmf

@rem | .3ds  | x | x | p |   |   | 3D Studio                               |

@rem | .ac   | x | x | x |   | x | AC3D                                    |
@rem | .acc  | x | x | x |   | x | AC3D with triangle strips (TORCS)       |
call make_plugin imp_ac3d

@rem | .ar   | x |   |   |   | x | Racer container files                   |
@rem | .dof  | x | x | p | x |   | Racer model files                       |
call make_plugin imp_ar

@rem | .ase  | x |   | p | x |   | ASCII Scene Exporter (3D Studio)        |
call make_plugin imp_ase

@rem | .b3d  | x | x |   |   |   | ?? (3D MetaFile format)                 |
@rem | .car  | x |   |   |   | x | VDrift car description                  |
@rem | .cob  | x | x |   |   |   | Caligari TrueSpace                      |

@rem | .dxf  | x |   |   |   |   | AutoCAD                                 |
@rem | .flt  | p |   |   |   |   | OpenFlight (in Progress, experimental)  |
@rem | .glb  | x | x | x | x |   | UltimateStunts models                   |
call make_plugin imp_glb
@rem | .iob  | x | x |   |   |   | Impulse Turbo Silver / Imagine          |
@rem | .iv   | p |   |   |   |   | SGI Inventor (ascii only)               |
@rem | .joe  | x |   | x | x |   | VDrift v3 .joe car models               |
call make_plugin imp_joe
@rem | .lcd  | x | x |   |   |   | LeoCAD                                  |
call make_plugin imp_leocad

@rem | .lw   | x | x |   |   |   | LightWave                               |
@rem | .lwb  | x | x |   |   |   | LightWave                               |
@rem | .lwo  | x | x |   |   |   | LightWave                               |
call make_plugin imp_lwo

@rem | .mb   | p |   |   |   | x | Maya (binary)                           |
@rem | .md2  | x |   |   |   |   | id Software Quake II                    |
@rem | .md3  | x | x | x | ? |   | id Software Quake III                   |
call make_plugin imp_md3

@rem | .mon  | p |   |   |   |   | Monzoom (Reflections) (experimental)    |
@rem | .nff  | x | x |   | x |   | Neutral File Format                     |
@rem | .obj  | x | x |   |   |   | Alias Wavefront Maya                    |
@rem | .prj  | x | ? |   |   |   | 3D Studio                               |
@rem | .q3o  | x | x | x |   |   | Quick3D Object                          |
@rem | .q3s  | x | x | x |   |   | Quick3D Scene                           |
call make_plugin imp_q3o
@rem | .r3   | p |   |   |   |   | Reflections 3 (experimental)            |
@rem | .r4   | p |   |   |   |   | Reflections 4 (experimental)            |
@rem | .stl  | p |   |   |   |   | Standard Tessellation Language          |
@rem | .stla | p |   |   |   |   | Standard Tessellation Language (ASCII)  |
@rem | .stlb | p |   |   |   |   | Standard Tessellation Language (binary) |
@rem | .wrl  | x | x |   |   |   | VRML World (VRML 1 only and incomplete) |
@rem +-------+---+---+---+---+---------------------------------------------+