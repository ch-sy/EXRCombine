# EXRCombine
Render Blender scenes at low sample rate and merge them to one high-quality image afterwards.

## Workflow

1. Set the output of your scenes so that all passes of all scenes you want are written to one multilayer EXR file. Use either the output settings or create a file output node in the compositor.
2. Render your scene(s) to one multilayer EXR file multiple times at low sample rate with different seed. For still images, simply click "animate seed" and render the whole animation.
3. Combine them with this tool `combine ./combined/layers.exr ./out/frame*.exr`.
4. Do the compositing in another scene. Load `./combined/layers.exr` as image source.
- You may do this for an arbitrary amount of EXR files with arbitrary layers. But less files make less work, so it is recommended to put all in one file or one file per scene.

## Advantages

- EXR is cool.
  - Images store image data in linear color space. This means that there will be no quality loss due to calculating with non-linear sRGB color data.
  - This works with non-color data as well (like the Depth- or Normal pass). No need to squeeze those values into RGB.
  - Use 16/32bit float values to store each color channel (instead of 8/16bit integer with PNG).
- Works great with Blender color management (e.g. Filmic Blender). You can change all settings after rendering in the compositor.
- Do your compositing on the high-quality final product, not just preview renders.

## Disadvantages

- Needs quite a lot of disk space.
- EXR output is not supported by SheepIt render farm.

## Installation

### For Arch Linux users

  git clone https://github.com/ch-sy/EXRCombine
  cd EXRCombine
  makepkg -i

### Other platforms

TODO

## Usage

TODO
