#include "render_pass_sprite_scene.hpp"

// SLUS-01334: draw wrapper calls only background and actor construction.
// Layer and actor ABI match the X-series sprite engine. X5 uses 16 OT
// buckets, not X6's 32; the SDK environments remain 0x14/0x5c bytes.
struct MMX5Scene {
    static constexpr uint32_t Draw=0x800272B4, End=0x800272D4;
    static constexpr uint32_t Camera=0x80027C5C, CameraInstruction=0x0C00A09E;
    static constexpr uint32_t Actor=0x80024C60, VSync=0x80062E48;
    static constexpr uint32_t Base=0x800C9328, Stride=0xB0, Buckets=16;
    static constexpr uint32_t CurrentBuffer=0x8009E794, Layers=0x8009A1F8;
    static constexpr uint32_t OffsetPackets=0x800D5188, ClipPackets=0x800D51C8;
    static constexpr uint32_t PutDrawEnv=0x80068C98, DrawOTag=0x80068C28, DrawSync=0x80068634;
    static constexpr uint32_t WaitReturn=0x80011E2C, PacketFloor=0x92000;
    static constexpr const char* Package="mmx5.enhancement.frame-interpolation";
};
PSX_MOD_CONSTRUCTOR(mmx5_register_frame_interpolation_plugin) {
    PSXSpriteSceneReplay<MMX5Scene>::install("mmx5.frame-interpolation");
}
