Mega Man X5 Native Scene Interpolation

This default-disabled mod redraws actors and scrolling layers at intermediate
positions through the stable mmx5.frame-interpolation activation plugin.
It uses the shared sprite-scene replay and guest-state restoration services.
HUD positions and sprite animation frames remain discrete. Teleports, new
objects, and discontinuous scene changes do not inherit stale motion.

It is presentation only. Guest VBlank, game logic, timers, and audio retain
their stock cadence, so the game does not run faster. Enabling the mod selects
the OpenGL presenter and disables vsync.

Output rate can follow the measured display refresh or use a fixed rate from
90 through 240 FPS. Display refresh is the default because presenting faster
than the panel can show adds cost without improving visible smoothness.

Credit

mstan - mod integration
