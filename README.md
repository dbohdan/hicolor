# HiColor

![A building with a dithered gradient of the sky behind it.
A jet airplane is taking off in the sky.](bordeaux-15bit.png)

*(The image above has 15-bit color.)*

HiColor is a program and a C library for converting images to 15- and 16-bit RGB color,
the color depth of old display modes known as [&ldquo;high color&rdquo;](https://en.wikipedia.org/wiki/High_color).
I wrote it because I wanted to create images with the characteristic high-color look.

## Contents

- [Description](#description)
- [Known bugs and limitations](#known-bugs-and-limitations)
- [Usage](#usage)
- [Building](#building)
- [Alternatives](#alternatives)
- [License](#license)

## Description

HiColor reduces images to two-byte 15- or 16-bit color.
In 15-bit mode images have 5 bits for each of red, green, and blue, with the last bit reserved.
In 16-bit mode green, the color the human eye is generally most sensitive to, is given 6 bits.

HiColor implements its own simple [file format](format.md) and converts between this format and PNG.
It can also convert standard PNG to standard PNG with only high-color color values.
(This simulates a roundtrip through HiColor without creating a temporary file.)
HiColor files have either the extension `.hic` or `.hi5` for 15-bit and `.hi6` for 16-bit respectively.

By default,
HiColor applies the [Bayer ordered dithering](https://en.wikipedia.org/wiki/Ordered_dithering) algorithm
to reduce the quantization error
(the difference between the original and the high-color pixel).
Historical software and hardware used it for dithering in high-color modes.
HiColor can also use [&ldquo;a dither&rdquo;](https://pippin.gimp.org/a_dither/) instead.
Dithering can be selected or disabled with command-line flags.

Quantized images compress better than their originals,
so HiColor can be a less-lossy alternative to the 256-color [pngquant](https://pngquant.org/).
Quantizing a PNG file to PNG preserves transparency (but does not quantize the alpha channel).
Conversion to and from the HiColor format does not preserve transparency.

The program is written in C with two external dependencies, libpng and zlib, and builds as a static binary.
It is known to work on
Linux (aarch64, i386, riscv64, x86_64),
FreeBSD,
NetBSD,
OpenBSD,
and Windows 98 Second Edition,
2000 Service Pack 4,
XP,
7,
and 10.

The library is a single C99 header file.
It is designed to be easy to understand and modify
at a cost to performance.
The design makes it unsuitable for real-time graphics.

## Known bugs and limitations

### Security

The command-line program (but not the library) was vulnerable to malicious PNG files
because it used a PNG library intended only for trusted input.
The vulnerabilities were fixed in version 0.6.0 by switching to libpng.

### PNG file size

PNG files produced by HiColor are not highly optimized.
Run them through [OptiPNG](http://optipng.sourceforge.net/) or [Oxipng](https://github.com/shssoichiro/oxipng) to significantly reduce their size.

### Generation loss

With Bayer dithering or no dithering, there is no [generation loss](https://en.wikipedia.org/wiki/Generation_loss) after the initial quantization.
Applying &ldquo;a dither&rdquo; repeatedly to the same image will result in generation loss.
In tests the loss converges to zero after 32 or 64 generations
(in 15-bit and 16-bit mode respectively).

HiColor 0.1.0&ndash;0.2.1 suffered from generation loss with Bayer dithering due to an implementation error.
The error was fixed in version 0.3.0.

## Usage

HiColor has a Git-style CLI.

The actions `encode` and `decode` convert images between PNG and HiColor's own image format.
`quantize` round-trips an image through the converter and outputs a standard 32-bit PNG.
Use `quantize` to create high-color images readable by other programs.
`info` prints information about a HiColor file: version (`5` for 15-bit or `6` for 16), width, and height.

```none
HiColor 1.0.1
Create 15/16-bit color RGB images.

usage:
  hicolor (encode|quantize) [-5|-6] [-a|-b|-n] [--] <src> [<dest>]
  hicolor decode <src> [<dest>]
  hicolor info <file>
  hicolor (version|help|-h|--help)

commands:
  encode           convert PNG to HiColor
  decode           convert HiColor to PNG
  quantize         quantize PNG to PNG
  info             print HiColor image version and resolution
  version          print version of HiColor, libpng, and zlib
  help             print this help message

options:
  -5, --15-bit     15-bit color
  -6, --16-bit     16-bit color
  -a, --a-dither   dither image with "a dither"
  -b, --bayer      dither image with Bayer algorithm (default)
  -n, --no-dither  do not dither image
```

## Building

### Debian/Ubuntu

```sh
sudo apt install -y build-essential graphicsmagick linpng-dev pkgconf tclsh zlib1g-dev
gmake test
```

### FreeBSD

```sh
sudo pkg install -y gmake GraphicsMagick pkgconf png tcl86
ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi
gmake test
```

### macOS

Install [Homebrew](https://brew.sh/).
Run the following commands in a clone of the HiColor repository.

```sh
brew install libpng tcl-tk
make test
```

### NetBSD

```sh
sudo pkgin -y install gmake GraphicsMagick pkgconf png tcl zlib
gmake test
```

### OpenBSD

```sh
doas pkg_add -I gmake GraphicsMagick pkgconf png tcl%8.6
ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
gmake test
```

### Windows

Install [MSYS2](https://www.msys2.org/).
Run the following commands in the MSYS2 mingw32 shell
in a clone of the HiColor repository.
This will build an x86 executable for Windows.

```sh
pacman -Syuu make mingw-w64-i686-gcc mingw-w64-i686-libpng mingw-w64-i686-pkgconf mingw-w64-i686-zlib tcl
make test
```

## Alternatives

I wrote HiColor because nothing seemed to support high color.
Actually,
[FFmpeg](https://www.madox.net/blog/2011/06/06/converting-tofrom-rgb565-in-ubuntu-using-ffmpeg/),
[GIMP](https://docs.gimp.org/2.10/en/gimp-filter-dither.html),
and
[ImageMagick](https://www.imagemagick.org/Usage/quantize/#16bit_colormap)
can reduce images to 15- and 16-bit color.
What differentiates HiColor is being a small dedicated tool and embeddable C library and having its own file format.

## License

MIT.

HiColor uses [libpng](http://www.libpng.org/pub/png/libpng.html) and [zlib](https://www.zlib.net/).
Follow the links for their respective licenses.

### Photos from Unsplash

[&ldquo;plane in flight&rdquo;](https://unsplash.com/photos/AwtncJT1qKs) (`bordeaux-15bit.png`) by olaf wisser.

[&ldquo;houses beside trees&rdquo;](https://unsplash.com/photos/PWBXQJ7PUkI) (`tests/photo.png`) by Orlova Maria.

#### License

> Unsplash grants you an irrevocable, nonexclusive, worldwide copyright license to download, copy, modify, distribute, perform, and use photos from Unsplash for free, including for commercial purposes, without permission from or attributing the photographer or Unsplash. This license does not include the right to compile photos from Unsplash to replicate a similar or competing service.
