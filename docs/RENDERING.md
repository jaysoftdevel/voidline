# VOIDLINE native renderer

VOIDLINE creates an OpenGL 3.3 core context through SDL2. The C++ renderer owns the vertex batches, shaders, textures, HDR framebuffers and pass order. Its 1200 × 600 gameplay canvas scales to the display. The HUD and menus render directly to the window after world post-processing.

1. A half-resolution scene target records parallax artwork, platforms and world objects for planar reflections. The surface shader samples the mirrored scene above the surface and applies ripple displacement, roughness and distance falloff.
2. The main HDR scene draws the backplate, hazard surface, metal terrain, characters and particles. Materials use geometric bevels, edge highlights, shaded armor and emissive strips.
3. A smaller additive light buffer accumulates bounded point and cone lights. The shader tests rays against nearby platform and actor boxes. High quality adds offset rays to soften shadow edges.
4. An atmospheric shader combines the light buffer with animated height fog. Fog color and in-scattering respond to the sector palette and nearby lights.
5. Bloom uses a soft-knee threshold, a four-level downsample pyramid and additive tent upsampling, followed by tone mapping. Performance mode uses two levels and fewer light samples.
6. The fixed-step simulation emits sparks, smoke and debris. Solid particles bounce off platforms and decelerate. Ambient motes and vent trails have independent visual lifetimes.

Platform faces use world-space material noise for fine grain, paint wear, brushed highlights and sector-specific corrosion. Backdrops, intermediate structures and foreground steel move at different rates. High quality scales the HDR world buffer up to 1920 × 960 as the display allows. Performance uses a 1200 × 600 target and a smaller bloom pyramid.

The uploaded gait reference informed heel contact, toe-off and swing. The simulation advances the gait by horizontal travel. Each stance foot locks to platform coordinates, including moving lifts. The renderer blends pose snapshots and solves two-segment legs. Sprint increases stride and body lean, and crouch reduces the collision height as well as the rendered stance. Jumps use a short push-off, a tucked airborne pose, variable lift and a damped landing response.

## Figures and animation

`src/figure.cpp` holds a shared side-view human rig used by the three operatives and the soldier enemies. It has a spine that follows the pelvis, a head that looks toward the aim, a neck, shoulders with pauldrons, two-bone arms and legs, hands and boots. Each operative has their own face, hair and headgear, and a distinct backpack. The torso carries plates, a belt, pouches, a cross strap and lamps, and the legs carry knee pads, greaves and boot soles.

Motion layers on top of the foot-locked gait. The body turns through a profile instead of snapping, breathes while idle, sinks and squashes on landing, recoils from each shot and flinches when hit. Arms follow the weapon grip. Soldiers carry their rifle low until a target is in range and then raise it. Defeated enemies tumble, bounce and settle before they fade. Drones bank and spin four-blade rotors, turrets track with a gimbal and kick back, and stalkers use a diagonal gait with a lashing tail and a crouch before a lunge.

## Stylized look

Actors draw after lighting and fog with heavy ink outlines, flat saturated armor shells, a scarf in the operative's color and sprite art for the weapons. The final pass applies a per-sector color grade, filmic contrast, light chromatic aberration, film grain and a vignette. Kills add a zoom punch and flash, damage adds a red edge, and bullet time adds a cool grade, speed streaks and a push-in. Shockwave rings, shell casings, motion streaks and outlined pop-up text reinforce impact. A mid-air flip rotates the whole body.

The sound mixer selects distinct footstep and combat variants, softens distant effects and limits peaks in the final mix. Weapon effects layer attack, body resonance, mechanism sounds and short stereo reflections. Foley layers add surface contact and gear movement.

## Rendering references

The shaders and renderer in this update were written for VOIDLINE. The following open-source projects informed the implementation.

- [LearnOpenGL Physically Based Bloom](https://github.com/JoeyDeVries/LearnOpenGL/tree/master/src/8.guest/2022/6.physically_based_bloom) informed the bloom pyramid and tent filter.
- [three.js Reflector](https://github.com/mrdoob/three.js/blob/dev/examples/jsm/objects/Reflector.js) informed the separate reflection render target and planar mirroring.
- [Godot fog processing](https://github.com/godotengine/godot/blob/master/servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl) informed density, transmittance and light scattering. VOIDLINE computes these in screen space for its 2D world.
- [SDL2](https://github.com/libsdl-org/SDL/tree/SDL2) supplies Linux window, input and audio services. The subset in platform.hpp follows SDL2's public ABI and license. The other system libraries use their public decoding and JSON ABIs.
