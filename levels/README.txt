VOIDLINE source level overrides

Model Studio publishes edited campaign/co-op layout files here only when changed.
The Linux/Windows builds copy *.vllevel.json beside the executable.
When a level is restored to its original layout, the editor removes its source
override; the next build removes that override from the runtime.
The original built-in stages remain in src/content.
