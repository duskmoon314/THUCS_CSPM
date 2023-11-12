const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    //  .default_target = .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu }
    const optimize = .ReleaseFast;

    {
        const timeit = b.addExecutable(.{
            .name = "timeit",
            .root_source_file = .{
                .path = "src/timeit.cpp",
            },
            .target = target,
            .optimize = optimize,
        });
        timeit.linkLibC();
        timeit.linkLibCpp();
        timeit.addIncludePath(.{ .path = "src" });
        timeit.addIncludePath(.{ .path = "lib/cxxopts" });

        b.installArtifact(timeit);
    }

    {
        const bandwidth = b.addExecutable(.{
            .name = "bandwidth",
            .root_source_file = .{
                .path = "src/bandwidth.cpp",
            },
            .target = target,
            .optimize = optimize,
        });
        bandwidth.linkLibC();
        bandwidth.linkLibCpp();
        bandwidth.addIncludePath(.{ .path = "src" });
        bandwidth.addIncludePath(.{ .path = "lib/cxxopts" });

        b.installArtifact(bandwidth);
    }
}
