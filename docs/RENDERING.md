# VOIDLINE native renderer

VOIDLINE creates an OpenGL 3.3 core context through SDL2. The C++ renderer owns the vertex batches, shaders, textures, HDR framebuffers and pass order. Its 1200 × 600 gameplay canvas scales to the display. The HUD and menus render directly to the window after world post-processing.

1. A half-resolution scene target records parallax artwork, platforms and world objects for planar reflections. The surface shader samples the mirrored scene above the surface and applies ripple displacement, roughness and distance falloff.
2. The main HDR scene draws the backplate, hazard surface, metal terrain, characters and particles. Materials use geometric bevels, edge highlights, shaded armor and emissive strips.
3. A smaller additive light buffer accumulates bounded point and cone lights. The shader tests rays against nearby platform and actor boxes. High quality adds offset rays to soften shadow edges.
4. An atmospheric shader combines the light buffer with animated height fog. Fog color and in-scattering respond to the sector palette and nearby lights.
5. Bloom uses a soft-knee threshold, a four-level downsample pyramid and additive tent upsampling, followed by tone mapping. Performance mode uses two levels and fewer light samples.
6. The fixed-step simulation emits sparks, smoke and debris. Solid particles bounce off platforms and decelerate. Ambient motes and vent trails have independent visual lifetimes.

Platform faces use world-space material noise for fine grain, paint wear, brushed highlights and sector-specific corrosion. Backdrops, intermediate structures and foreground steel move at different rates. High quality scales the HDR world buffer up to 1920 × 960 as the display allows. Performance uses a 1200 × 600 target and a smaller bloom pyramid.

## Mouse-driven perspective

The view eases toward the cursor using a small projective tilt and roll. Platform tops, end caps and rear rails converge toward a movable vanishing point. Backdrops and foreground steel respond at different rates. The world and its effects pass through the same projection, while the HUD and reticle draw in screen space. Aiming reverses the displayed transform and sends a world-space target in co-op. Each client computes its own view. Disable the effect with **Screen shake and camera motion** in Settings.

The camera reserves a fixed edge crop while camera motion is enabled. Vertical sweeps ease through center at a consistent base zoom. The letterbox margins hold the nearest edge angle until the pointer leaves the window.

## Chapter cutscenes

Five timed shots per chapter combine camera movement, perspective-textured floors, fog and animated props. Walking poses track distance and hold the supporting foot against the deck through each stride. The operative rig renders gestures, the hand carrying a recorder and reactions between the two characters. Chapter-specific animations show airlocks, floodwaters, terminal signals, civilian boarding and the cutter's arrival or departure.

The opening and chapter scenes share an HDR compositing pass with soft light shafts, mild peripheral defocus, glow and smoothly blended grain. Adjacent shots dissolve across two scene buffers. Captions and controls render afterward at display resolution.

The uploaded gait reference informed heel contact, toe-off and swing. The simulation advances the gait by horizontal travel. Each stance foot locks to platform coordinates, including moving lifts. The renderer blends pose snapshots and solves two-segment legs. Sprint increases stride and body lean, and crouch reduces the collision height as well as the rendered stance. Jumps use a short push-off, a tucked airborne pose, variable lift and a damped landing response.

## Version 2.1 pipeline

- **Formats:** HDR targets use `R11F_G11F_B10F`, which halves bandwidth against the earlier RGBA16F and loses no range that the scene needs. The tone-mapped result goes into one `RGBA8` target.
- **Tone mapping:** [AgX](https://github.com/sobotka/AgX) display transform with the polynomial sigmoid approximation, the same family of transform that Blender 4 and Godot 4 adopted. A punch and saturation step follows.
- **Anti-aliasing:** [FXAA 3.11](https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf) (Timothy Lottes) on the tone-mapped image, then film grain.
- **Relief lighting:** The atmosphere pass reads the backplate as a height field and the light map's gradient as a light direction, so lit faces catch light and cavities stay dark. This brings a normal-mapped look to the painted plates without extra textures.
- **Actor lighting:** Figures are drawn after lighting and tinted by the lights that reach them.
- **Volumetric shafts:** Lamp cones are drawn before fog and bloom, so they pick up haze and glow.
- **Soft clouds:** Smoke and dust use a generated fractal noise sprite.
- **Adaptive resolution:** The world buffer follows frame time. It steps down quickly when frames exceed about 20 ms and back up slowly when there is headroom. The window and interface always render at native resolution.
- **Cheaper state changes:** Uniform locations are cached and asset lookups avoid string allocation.

Run `./run.sh --smoke-test --scene 4 --width 1920 --frames 600` to see an average frame-time breakdown.

## Figures and animation

The shared side-view human rig lives in `src/render/figures/`: `rig.cpp` builds the skeleton, `legs.cpp`, `torso.cpp`, `head.cpp`, `headgear.cpp` and `arms.cpp` draw the parts and `humanoid.cpp` ties them together. The three operatives and the soldier enemies use it. Each operative has their own face, hair, headgear and backpack.

**Arms follow human anatomy.** An arm is a shoulder, an upper arm of 11.6 units and a forearm of 10.6 units joined by an elbow that only bends forward, like a real one. A free arm swings against the same-side leg: about 20 degrees at a walk and 40 to 45 at a run, further forward than back, and the elbow flexes more as the arm comes forward. The shoulder blade glides with the arm. A reaching arm solves the elbow from the hand position, keeps it below and behind the line from shoulder to hand and never stretches past its reach. The elbow pad sits on the outside of the bend and creases show on the inside as it folds. Hands are closed fists around a grip or relaxed open hands with curled fingers and a thumb. Operatives carry the rifle low while walking, with the support arm swinging freely, and raise it with both arms when aiming, firing or reloading.

Motion layers on top of the foot-locked gait. The body turns through a profile, breathes while idle, squashes on landing, recoils from each shot and flinches when hit. Soldiers carry the rifle low until a target is in range.

**Hit feedback and deaths.** Enemies store their wounds relative to their body, so holes follow them as they move, fall and tumble. Soldiers react by body part. Dying soldiers collapse in place (the rig gains `fall`, which bends the knees and spine and lets the arms go limp), the rifle is spawned as a separate particle and the body topples about its feet. Other types have their own death poses in `enemy_*.cpp`: rotors shear off drones, turret barrels are blown off, stalker legs curl in and guardians smolder and sink. The matching simulation code is in `src/game/death_*.cpp`.

## Stylized look

Actors draw after lighting and fog with heavy ink outlines, flat saturated armor shells, a scarf in the operative's color and sprite art for the weapons. The final pass applies a per-sector color grade, filmic contrast, light chromatic aberration, film grain and a vignette. Kills add a zoom punch and flash, damage adds a red edge, and bullet time adds a cool grade, speed streaks and a push-in. Shockwave rings, shell casings, motion streaks and outlined pop-up text reinforce impact. A mid-air flip rotates the whole body.

The sound mixer selects distinct footstep and combat variants, softens distant effects and limits peaks in the final mix. Weapon effects layer attack, body resonance, mechanism sounds and short stereo reflections. Foley layers add surface contact and gear movement.

## Rendering references

The shaders and renderer in this update were written for VOIDLINE. The following open-source projects informed the implementation.

- [LearnOpenGL Physically Based Bloom](https://github.com/JoeyDeVries/LearnOpenGL/tree/master/src/8.guest/2022/6.physically_based_bloom) informed the bloom pyramid and tent filter.
- [three.js Reflector](https://github.com/mrdoob/three.js/blob/dev/examples/jsm/objects/Reflector.js) informed the separate reflection render target and planar mirroring.
- [Godot fog processing](https://github.com/godotengine/godot/blob/master/servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl) informed density, transmittance and light scattering. VOIDLINE computes these in screen space for its 2D world.
- [SDL2](https://github.com/libsdl-org/SDL/tree/SDL2) supplies Linux window, input and audio services. The subset in platform.hpp follows SDL2's public ABI and license. The other system libraries use their public decoding and JSON ABIs.
