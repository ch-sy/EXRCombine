#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <stdexcept>
#include <set>
#include <vector>
#include <string>
#include <stdio.h>
#include <omp.h>

using namespace Imf;
using namespace std;

class Combine {
private:
    half*   pixelDst        = nullptr;
    half*   pixelSrc        = nullptr;
    size_t  subpixelCount   = 0;
    int     imageCount      = 0;
    int     width           = 0;
    int     height          = 0;
    vector<string>  channelNames;

    FrameBuffer getFramebuffer(half* destination) {
        FrameBuffer fb;
        int i, xStride, yStride;

        xStride = sizeof(half) * channelNames.size();
        yStride = xStride * width;

        for(i = 0; i < channelNames.size(); i++){
            long base = (sizeof(half)*i)+(long)destination;
            fb.insert(channelNames[i], Slice(HALF,(char*)base,xStride,yStride));
        }
        return fb;
    }

public:
    void addImage(char* filename) {
        // Read the image
        InputFile img(filename);

        // Get the size of the input image
        Imath::Box2i dw = img.header().displayWindow();
        int srcWidth = dw.max.x - dw.min.x + 1;
        int srcHeight = dw.max.y - dw.min.y + 1;

        if(imageCount == 0) {
             // Get channels
            const ChannelList &channels = img.header().channels();
            set<string> layerNames;
            channels.layers (layerNames);
            for(set<string>::const_iterator i = layerNames.begin(); i != layerNames.end(); ++i) {
                ChannelList::ConstIterator layerBegin, layerEnd;
                channels.channelsInLayer(*i, layerBegin, layerEnd);
                for(ChannelList::ConstIterator j = layerBegin; j != layerEnd; ++j){
                    channelNames.push_back(j.name());
                }
            }

            // First image
            width = srcWidth;
            height = srcHeight;
            subpixelCount = width * height * channelNames.size();
            pixelDst = new half[subpixelCount];
            pixelSrc = new half[subpixelCount];

            // Read the image into the destination buffer
            img.setFrameBuffer(getFramebuffer(pixelDst));
            img.readPixels(dw.min.y, dw.max.y);
        } else {
            // Append image
            if(srcWidth != width || srcHeight != height)
                throw runtime_error("Image sizes are not equal!");

            img.setFrameBuffer(getFramebuffer(pixelSrc));
            img.readPixels(dw.min.y, dw.max.y);

            // Add the pixel
            #pragma omp parallel for
            for(size_t i = 0; i < subpixelCount; i++)
                pixelDst[i] += pixelSrc[i];
        }

        imageCount++;
    }

    void writeImage(char* filename) {
        // Normalise pixel
        half mult = half(1) / half(imageCount);
        #pragma omp parallel for
        for(size_t i = 0; i < subpixelCount; i++)
            pixelDst[i] *= mult;

        // Create header
        Header header (width, height);
        for(auto chan : channelNames)
            header.channels().insert(chan, Channel(HALF));

        OutputFile img(filename, header);
        img.setFrameBuffer(getFramebuffer(pixelDst));

        img.writePixels (height); 
    }

    ~Combine() {
        // Free the reserved space
        delete[] pixelDst;
        delete[] pixelSrc;
    }
};

int main(int argc, char **argv) {
    cout << "Combine images..." << endl;
    
    if(argc < 3){
        cout << "Too few arguments!" << endl;
        return 0;
    }
    
    double wtime = omp_get_wtime();
    Combine flauschImage;
    
    for(int i = 2; i < argc; i++){
        try {
            printf("[% 3i%] Loading %s\n", (i - 2) * 100 / (argc - 1), argv[i]);
            flauschImage.addImage(argv[i]);
        } catch (const std::exception &exc) {
            cerr << exc.what() << std::endl;
        }
    }
        
    try {
        printf("[% 3i%] Writing %s\n", (argc - 2) * 100 / (argc - 1), argv[1]);
        flauschImage.writeImage(argv[1]);
    } catch (const std::exception &exc) {
        cerr << exc.what() << std::endl;
    }
    cout << "[100%] Finished in " << omp_get_wtime() - wtime << " seconds!" << endl;
    return 0;
}