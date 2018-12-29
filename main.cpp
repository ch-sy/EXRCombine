#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfChannelList.h>
#include <vector>
#include <array>
#include <iostream>
#include <set>
#include <string>
#include <stdexcept>
#include <stdio.h>  /* defines FILENAME_MAX */
// #define WINDOWS  /* uncomment this line to use it for windows.*/ 
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  GetCurrentDir( buff, FILENAME_MAX );
  std::string current_working_dir(buff);
  return current_working_dir;
}

#define CLAMP(x, min_x, max_x) ((x) < (min_x) ? (min_x) : ((x) > (max_x) ? (max_x) : (x)))

using namespace Imf;
using namespace std;

class RenderImage : public InputFile {
private:
    string      name;
    set<string> layerNames; // Name of the layers
    size_t      layerCount; // Count of the layers
    size_t      width;      // Width of the image
    size_t      height;     // Height of the image
    size_t      pixelArraySize;
    Rgba*       pixels;     // Pixel buffer of the image
    ChannelList channels;
    
    int getPixelPos(int x, int y, int layerIndex) {
        return (y * width + x) * layerCount + layerIndex;
    }
    
    

public:
    RenderImage(const char fileName[]) : InputFile(fileName) {
        this->name = string(fileName);
        
        // Get the size of the input image
        Imath::Box2i dw = header().displayWindow();
        width = dw.max.x - dw.min.x + 1;
        height = dw.max.y - dw.min.y + 1;
        
        // Get layer names
        channels = header().channels();
        channels.layers(layerNames);
        layerCount = layerNames.size();
        
        // Reserve pixel array
        pixelArraySize = width * height * layerCount;
        cout << "Bytes reserved: " << pixelArraySize*sizeof(Rgba) << endl;
        pixels = new Rgba[pixelArraySize];
        
        FrameBuffer frameBuffer;
        
        // Write the image data into the pixel buffer
        
        int layerIndex = 0;
        for(set<string>::const_iterator i = layerNames.begin(); i != layerNames.end(); ++i) {
            
            // Print debug infos
            cout << "layer " << *i << endl;
        
            ChannelList::ConstIterator layerBegin, layerEnd;
            channels.channelsInLayer(*i, layerBegin, layerEnd);
            int chan = 4;
            for(ChannelList::ConstIterator j = layerBegin; j != layerEnd; ++j){
                chan--;
                int xStride = sizeof(Rgba) * layerCount;
                int yStride = sizeof(Rgba) * width * layerCount;
                long base = (sizeof(half)*chan+sizeof(Rgba)*layerIndex)+(long)pixels-dw.min.x-dw.min.y*xStride;
                
                cout << "\tchannel " << j.name() << " base &p+" << (char*)(base)-(char*)pixels << " xStride " << xStride << " yStride " << yStride << endl;
                frameBuffer.insert(j.name(),Slice(HALF,(char*)base,xStride,yStride));
                
            }
            layerIndex++;
            
        }
        setFrameBuffer(frameBuffer);
        readPixels(dw.min.y, dw.min.y);
    }
    
    ~RenderImage() {
        // Delete the pixel array
        delete[] pixels;
    }
    
    // getPixel trys to get the pixel at the specified position and layer. Clamps the position if it isn't in bounds.
    Rgba getPixel(int x, int y, int layerIndex) {
        // Check if the pixel exists in its bounds
        x = CLAMP(x, 0, width - 1);
        y = CLAMP(y, 0, height - 1);
        
        if(layerIndex >= 0 && layerIndex < layerCount)
            return pixels[getPixelPos(x, y, layerIndex)];
        
        throw "Layer doesn't exists!";
    }
    
    // getLayerIndex resolves the layername to an index
    size_t getLayerIndex(string str) {
        // TODO: Get the layer index
        return 0;
    }
    
    // getSize retrieves the width and height of this image
    void getSize(size_t &width, size_t &height) {
        width = this->width;
        height = this->height;
    }
    
    Rgba getNearestNeighbour(float x, float y, size_t layerIndex) {
        return getPixel(x * width, y * height, layerIndex);
    }
    
    Rgba getBilinear(float x, float y, size_t layerIndex) {
        // TODO
        throw "Bilinear not implemented";
    }
    
    Rgba getBicubic(float x, float y, size_t layerIndex) {
        // TODO
        throw "Bicubic not implemented";
    }
    
    set<string>* getLayerNames() {
        return &layerNames;
    }
    
    string getName() {
        return name;
    }
};


class MergeImage {
    
private:
    size_t                  layerCount;     // Count of the layers of the input images
    set<string>             layerNames;     // All layer names of the input images
    size_t                  width;          // Output width of the resulting image
    size_t                  height;         // Output height of the resulting image
    Rgba*                   pixels;         // Output pixels of the resulting image
    vector<RenderImage*>    inputImages;    // Input images taking account in the render pipeline
    
    // composeLayers mix all layers together per pixel
    void composeLayers(const Rgba inPixel[], Rgba &outPixel, size_t layerCount) {
        // Zero the output pixel
        outPixel = {0, 0, 0, 0};
        
        // Example: Build the median of all layers
        for(size_t layer = 0; layer < layerCount; ++layer) {
            outPixel.r += inPixel[layer].r / half(layerCount);
            outPixel.g += inPixel[layer].g / half(layerCount);
            outPixel.b += inPixel[layer].b / half(layerCount);
            outPixel.a += inPixel[layer].a / half(layerCount);
        }
    }
    
    // mergeImage merge all images together
    void mergeImage(const Rgba inPixel[], Rgba &outPixel, size_t imageCount) {
        // The image count multiplier
        half mult = 1.0f / half(imageCount);
        
        // Zero the output pixel
        outPixel = {0, 0, 0, 0};
        
        // Example: Build the median of all images
        for(size_t img = 0; img < imageCount; ++img) {
            outPixel.r += inPixel[img].r * mult;
            outPixel.g += inPixel[img].g * mult;
            outPixel.b += inPixel[img].b * mult;
            outPixel.a += inPixel[img].a * mult;
        }
    }
    
    // render renders the output
    void render() {
        size_t arSize = width * height;
        
        // Free the memory of the previous pixels
        if(pixels != nullptr)
            delete[] pixels;
        
        pixels = new Rgba[arSize];
        cout << "Bytes reserved: " << arSize * sizeof(Rgba) << endl;
        
        size_t imageCount = inputImages.size();
        
        Rgba* bufferLayer = new Rgba[layerCount];
        Rgba* bufferImage = new Rgba[imageCount];
        
        // Process images
        for(size_t iy = 0; iy < height; iy++)
        {
            for(size_t ix = 0; ix < width; ix++)
            {
                for(int layer = 0; layer < layerCount; layer++)
                {
                    size_t img = 0;
                    for(auto image : inputImages) {
                        bufferImage[img++] = image->getPixel(ix, iy, layer);
                    }
                }
                composeLayers(bufferLayer, pixels[ix+iy*width], layerCount);
            }
        }
            
        delete[] bufferLayer;
        delete[] bufferImage;
    }
    
public:
    MergeImage(size_t width = 0, size_t height = 0) {
        this->width   = width;
        this->height  = height;
        this->pixels  = nullptr;
    }
    
    ~MergeImage(){
        // Free memory
        if(pixels != nullptr) delete[] pixels;
        for(auto image : inputImages) delete image;
    }
    
    // addInputImage adds an image to the render pipeline
    void addInputImage(const char fileName[]) {
        RenderImage* inputImage = new RenderImage(fileName);
        if(width == 0 || height == 0)
            inputImage->getSize(width, height);
        inputImages.push_back(inputImage);
        layerNames.insert(inputImage->getLayerNames()->begin(), 
                          inputImage->getLayerNames()->end());
        layerCount = layerNames.size();
    }
    
    // writeOutputImage renders and writes the image to disk
    void writeOutputImage(const char fileName[]) {
        render();
        RgbaOutputFile outputImage(fileName, width, height, Imf::WRITE_RGBA);
        outputImage.setFrameBuffer(pixels, 1, width);
        outputImage.writePixels(height);
    }
    
    // print shows infos about the resulting image
    void print() {
        cout << "Width: \t" << width << endl;
        cout << "Height:\t" << height << endl;
        cout << "Images:" << endl;
        for(auto image : inputImages) {
            size_t inpWidth;
            size_t inpHeight;
            image->getSize(inpWidth, inpHeight);
            cout << "- " << image->getName() << "\t" << inpWidth << "x" << inpHeight << endl;
            cout << "- first pixels: " << endl;
            for(int i = 10; i < 20; i++){
                cout << image->getPixel(i,0,0).r << "|";
                cout << image->getPixel(i,0,0).g << "|";
                cout << image->getPixel(i,0,0).b << "|";
                cout << image->getPixel(i,0,0).a << endl;
            }
        }
        cout << "Layers:" << endl;
        for(set<string>::const_iterator i = layerNames.begin(); i != layerNames.end(); ++i) {
            cout << "- " << *i << endl;
            
        }
    }
};


int main(int argc, char **argv) {
    
    /*cout << "Commands:" << endl;
    cout << "add   <imagefile>\tAdd the image to the render pipeline" << endl;
    cout << "write <imagefile>\tRender and write the image to disk" << endl;
    cout << "print            \tShows infos about the resulting image" << endl;
    cout << "exit             \tExit the programm" << endl << endl;*/
    
    cout << "EXRCombine" << endl;
    
    if(argc < 3){
        cout << "Too few arguments!" << endl;
        return 0;
    }
    
    string command;
    string file;
    MergeImage flauschImage;
    
    for(int i = 2; i < argc; i++){
        file = GetCurrentWorkingDir() + "/" + argv[i];
        try {
            flauschImage.addInputImage(file.c_str());
            cout << "Image added to the render pipeline!" << endl;
        }catch (const std::exception &exc){
            cerr << exc.what() << std::endl;
        }
    }
    
    flauschImage.print();
        
    file = GetCurrentWorkingDir() + "/" + argv[1];
    try {
        flauschImage.writeOutputImage(file.c_str());
        cout << "Image rendered and written to disk!" << endl;
    } catch (const std::exception &exc){
            cerr << exc.what() << std::endl;
    }
    
    
    /*while(true){
        cout << ">> ";
        cin >> command;
        getchar();
        if(command.compare("exit") == 0) {
            break;
        } else if(command.compare("add") == 0) {
            cin >> file;
            getchar();
            file = GetCurrentWorkingDir() + "/" + file;
            try {
                flauschImage.addInputImage(file.c_str());
                cout << "Image added to the render pipeline!" << endl;
            } catch(...) {
                cout << "Error by adding the image!" << endl;
            }
            
        } else if(command.compare("write") == 0){
            cin >> file;
            getchar();
            file = GetCurrentWorkingDir() + "/" + file;
            try {
                flauschImage.writeOutputImage(file.c_str());
                cout << "Image rendered and written to disk!" << endl;
            } catch(...) {
                cout << "Error by rendering or writing!" << endl;
            }
        } else if(command.compare("print") == 0){
            flauschImage.print();
        } else {
            cout << "Unknown command!" << endl; 
        }
    }*/
    
    return 0;
}
