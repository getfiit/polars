// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "polars",
    platforms: [
        .iOS(.v9),
        .macOS(.v10_10),
        .tvOS(.v9),
        .watchOS(.v2)
    ],
    products: [
        .library(
            name: "polars",
            targets: [ "polars" ])
    ],
    dependencies: [
       	.package(name: "armadillo", url: "https://github.com/getfiit/armadillo-code.git", .branch("10.7.x"))
    ],
    targets: [
		.target(
			name: "polars",
            dependencies: [ "armadillo" ],
            path: "src/cpp/",
            exclude: [
                "polars/CMakeLists.txt"
            ],
            publicHeadersPath: "./",
            cxxSettings: [
                .unsafeFlags([
                    "-Wno-modules-import-nested-redundant",
                    "-Wno-shorten-64-to-32"
                ])
            ]
        )
    ],
    cxxLanguageStandard: .cxx14
)

