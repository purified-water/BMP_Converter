#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

//Cấu trúc Header
struct Header {
    char signature[2];   //Chữ kí
    int fileSize;        //Kích thước file
    int reserved;        //Phần dành riêng
    int dataOffset;      //Địa chỉ phần bắt đầu lưu dữ liệu điểm ảnh
};
//Cấu trúc màu BGR/ABGR cho ảnh 24/32 bit
struct colorPix {
    unsigned char A;
    unsigned char B;
    unsigned char G;
    unsigned char R;
};
//Cấu trúc bảng màu cho ảnh 8bit
struct colorTable {
    uint8_t B;
    uint8_t G;
    uint8_t R;
    uint8_t Reserved;
};
//Cấu trúc DIB
struct DIB {
    int dibSize;    //Kích thước phần DIB
    int width;      //Chiều rộng
    int height;     //Chiều dài
    short planes;   //Lớp màu
    short bpp;      //Số bit trong 1 pixel
    int compression;  //Cách nén ảnh
    unsigned int imageSize;     //Kích thước phần pixel data
};
//Cấu trúc BMPImage
struct BMPImage {
    Header header;
    DIB dib;
    char* pDIBReserved;     //Con trỏ tới vùng nhớ lưu phần dư của DIB
    colorTable colorTable[256];    //Bảng màu (dành cho ảnh <= 8 bitsperpixel)
    char* pImageData;   //Con trỏ tới vùng nhớ lưu pixel data
};


//Hàm đọc file bmp
int readBMP(const char* filename, BMPImage& bmp) {
    ifstream fin(filename, ios::binary | ios::in);
    if (!fin)
        return 0;
    //Đưa con trỏ về vị trí đầu file
    fin.seekg(0, fin.beg);
    //Đọc header (14 bytes)
    fin.read((char*)&bmp.header, 14);
    //Đọc DIB (40 bytes)
    fin.read((char*)&bmp.dib, 40);
    //Kiểm tra phần thừa
    if (bmp.dib.dibSize > 40) {
        //Kích thước phần dư
        int remainder = bmp.dib.dibSize - 40; //Trừ đi phần đã đọc
        //Nếu dư thì cấp phát bộ nhớ
        bmp.pDIBReserved = new char[remainder];
        //Đọc dữ liệu từ file cho phần dư 
        fin.read(bmp.pDIBReserved, remainder);
    }
    else
        fin.read(bmp.pDIBReserved, 0);
    //Đọc bảng màu nếu bpp bé hơn bằng 8bpp
    if (bmp.dib.bpp <= 8) {
        fin.read((char*)bmp.colorTable, sizeof(bmp.colorTable));
    }
    //Cấp phát vùng nhớ cho phần pixel data
    bmp.pImageData = new char[bmp.dib.imageSize];
    //Đọc phần dữ liệu điểm ảnh
    fin.read((char*)bmp.pImageData, bmp.dib.imageSize);

    fin.close();
    return 1;

}
//Hàm ghi file bmp
int writeBMP(const char* filename, BMPImage bmp) {
    ofstream fout(filename, ios::binary | ios::out);
    if (!fout)
        return 0;
    //Đưa con trỏ về vị trí đầu file
    fout.seekp(0, fout.beg);
    //Ghi header
    fout.write((char*)&bmp.header, 14);
    //Ghi dib
    fout.write((char*)&bmp.dib, 40);
    //Ghi phần dư dib (nếu có)
    if (bmp.dib.dibSize > 40)
        fout.write(bmp.pDIBReserved, bmp.dib.dibSize - 40);
    //Nếu bpp <= 8 thì ghi phần bảng màu vào
    if (bmp.dib.bpp <= 8) {
        fout.write((char*)bmp.colorTable, sizeof(bmp.colorTable));
    }
    //Ghi phần dữ liệu điểm ảnh
    if (bmp.pImageData == NULL)
        return 0;

    fout.write((char*)bmp.pImageData, bmp.dib.imageSize);
    //Đóng file
    fout.close();

    return 1;

}
//Hàm chuyển đổi sang ảnh 8 bit
int to8Bit(BMPImage srcBmp, BMPImage& desBmp) {
    if ((srcBmp.dib.bpp != 24 && srcBmp.dib.bpp != 32) || srcBmp.pImageData == NULL)
        return 0;
    //Gán các thông tin quan trọng
    desBmp.header = srcBmp.header;
    desBmp.dib = srcBmp.dib;
    //cho desBpp = 8
    desBmp.dib.bpp = 8;
    //Gán pDIBReserved nếu có
    if (desBmp.dib.dibSize > 40) {
        int remainder = desBmp.dib.dibSize - 40;
        desBmp.pDIBReserved = new char[remainder];
    }
    //Cho kích thước = trị tuyệt đối (để tính toán)
    int height = abs(srcBmp.dib.height);
    int width = abs(srcBmp.dib.width);

    //Tính tổng số byte của 1 dòng width
    int lineBytes = 0;
    //Nếu bpp = 24 bits/pixel thì nhân 3
    if (srcBmp.dib.bpp == 24)
        lineBytes = width * 3;
    //Nếu bpp = 32 bits/pixel thì nhân 4     
    if (srcBmp.dib.bpp == 32)
        lineBytes = width * 4;

    int newLineBytes = lineBytes;
    //Kiểm tra tổng số byte trên 1 dòng có là bội số của 4 không
    if (lineBytes % 4 != 0) {
        //Nếu chia 4 dư khác 0 thì
        //Cho số chia + số bị chia - số dư để được số byte là bội của 4
        newLineBytes = lineBytes + 4 - (lineBytes % 4);
    }

    //Tính padding byte (32bpp không có padding bytes)
    int padding = newLineBytes - lineBytes;
    //Tính kích thước phần pixel data
    desBmp.dib.imageSize = (width * height * desBmp.dib.bpp) / 8 + padding * height;
    //Cấp phát vùng nhớ cho pixel data của ảnh desBmp
    desBmp.pImageData = new char[desBmp.dib.imageSize];
    //Con trỏ pRow trỏ đến pImageData
    char* pSrcRow = srcBmp.pImageData;
    char* pDesRow = desBmp.pImageData;
    //Số byte của 1 pixel ảnh
    int nSrcByteInPix = srcBmp.dib.bpp / 8;
    int nDesByteInPix = desBmp.dib.bpp / 8;
    //Số byte trên 1 dòng của ảnh
    int nSrcByteInRow = (width * nSrcByteInPix) + padding;
    int nDesByteInRow = (width * nDesByteInPix) + padding;
    //Con trỏ từng pixel
    char* pSrcPix;
    char* pDesPix;
    //Tạo bảng màu cho ảnh 8 bit
    //vì khi chuyển từ bpp cao hơn sang 8 bit
    //Dữ liệu màu bị mất đi, ta cần tạo bảng màu mới cho ảnh
    for (int i = 0; i < 256; i++) {
        desBmp.colorTable[i].B = i;
        desBmp.colorTable[i].G = i;
        desBmp.colorTable[i].R = i;
        desBmp.colorTable[i].Reserved = 0;
    }
    //Vòng lặp đi qua từng dòng và cột
    for (int y = 0; y < height; y++) {
        //Khởi tạo biến màu
        //Vì là 8 bit nên ta dùng uint8_t
        uint8_t B, G, R;
        uint8_t aveColor;
        //Tại dòng thứ y: đi qua từng pixel trên dòng
        //Con trỏ pDesPix nhảy vào từng pixel
        pSrcPix = pSrcRow;
        pDesPix = pDesRow;
        //Truy xuất từng byte của mỗi pixel
        for (int x = 0; x < width; x++) {
            if (srcBmp.dib.bpp == 24) {
                B = pSrcPix[0];
                G = pSrcPix[1];
                R = pSrcPix[2];
            }
            if (srcBmp.dib.bpp == 32) {
                B = pSrcPix[1];
                G = pSrcPix[2];
                R = pSrcPix[3];
            }
            //Tính trung bình cộng 3 màu R G B
            aveColor = (B + G + R) / 3;
            //Gán giá trị màu cho từng pixel pDesPix đi vào
            pDesPix[0] = aveColor;
            //p...Pix nhảy qua pixel kế tiếp 
            //= công thêm 1 khoảng số byte trong 1 pixel    
            pSrcPix += nSrcByteInPix;
            pDesPix += nDesByteInPix;
        }
        //Con trỏ nhảy qua dòng tiếp theo = cộng thêm 1 lượng
        //byte trên 1 dòng (kể cả padding)
        pSrcRow += nSrcByteInRow;
        pDesRow += nDesByteInRow;
    }
    return 1;
}
//Hàm tìm màu trung bình
unsigned char findAveColor(DIB dib, colorPix& p, char* pSrcRow, int nSrcByteInRow, int nSrcByteInPix, int S) {
    unsigned int aveCol = 0;
    unsigned int BGR;
    //Xét giá trị
    switch (dib.bpp) {
        //Trường hợp bpp == 8
    case 8: {
        aveCol = 0;
        for (int i = 0; i < S; i++) {

            char* pPix = pSrcRow;

            for (int j = 0; j < S; j++) {
                //Tính tổng các giá trị của pixel trong ô vuông
                aveCol += (unsigned char)pPix[0];
                //Di chuyển pPix sang pixel tiếp theo
                pPix += nSrcByteInPix;
            }
            //Di chuyển pSrcRow sang dòng tiếp theo 
            pSrcRow += nSrcByteInRow;
        }
        //Gán lại giá trị aveCol = trung bình cộng của tổng các màu
        //Ép kiểu lại thành unsigned char (từ unsigned int)
        aveCol = (unsigned char)(aveCol / (S * S));
        //Trả lại giá trị aveCol (unsigned char)
        return aveCol;
        break;
    }
          //Trường hợp bpp == 24
    case 24: {
        //Khởi tạo mảng BGR và gán giá trị 0 để tránh bị tràn 
        //dữ liệu khi tính toán
        unsigned int BGR[3] = { 0 };
        p.B = p.G = p.R = 0;
        for (int i = 0; i < S; i++) {
            //pPix trỏ đến từng dòng đang xét
            char* pPix = pSrcRow;

            for (int j = 0; j < S; j++) {
                //Cộng lần lượt các giá trị màu B G R
                BGR[0] += (unsigned char)pPix[0];
                BGR[1] += (unsigned char)pPix[1];
                BGR[2] += (unsigned char)pPix[2];
                //Di chuyển pPix sang pixel tiếp theo
                pPix += nSrcByteInPix;
            }
            //Di chuyển pSrcRow sang dòng tiếp theo 
            pSrcRow += nSrcByteInRow;
        }
        //Tính trung bình cộng cho từng giá trị màu
        //Ép kiểu lại thành unsigned char
        BGR[0] = (unsigned char)(BGR[0] / (S * S));
        BGR[1] = (unsigned char)(BGR[1] / (S * S));
        BGR[2] = (unsigned char)(BGR[2] / (S * S));
        //Gán lại cho p.B/G/R
        p.B = BGR[0];
        p.G = BGR[1];
        p.R = BGR[2];
        break;
    }
           //Trường hợp bpp == 32
           //Tương tự như case 24, chỉ khác ở chỗ ảnh 32bit sẽ có thêm biến Alpha
           //Nên ta sẽ gán, cộng và tính trung bình tương ứng cho 4 giá trị A B G R
    case 32: {
        unsigned int BGR[4] = {0};
        p.A = p.B = p.G = p.R = 0;
        for (int i = 0; i < S; i++) {

            char* pPix = pSrcRow;
            for (int j = 0; j < S; j++) {
                BGR[0] += (unsigned char)pPix[0];
                BGR[1] += (unsigned char)pPix[1];
                BGR[2] += (unsigned char)pPix[2];
                BGR[3] += (unsigned char)pPix[3];

                pPix += nSrcByteInPix;
            }
            pSrcRow += nSrcByteInRow;
        }
        BGR[0] = (unsigned char)(BGR[0] / (S * S));
        BGR[1] = (unsigned char)(BGR[1] / (S * S));
        BGR[2] = (unsigned char)(BGR[2] / (S * S));
        BGR[3] = (unsigned char)(BGR[3] / (S * S));
        p.A = BGR[0];
        p.B = BGR[1];
        p.G = BGR[2];
        p.R = BGR[3];
        break;
    }
    default:
        break;

    }
    return 1;


}
//Hàm scale ảnh xuống 1 tỉ lệ cho trước
int zoomIn(BMPImage srcBmp, BMPImage& desBmp, int S) {
    colorPix p;
    if ((srcBmp.dib.bpp != 24 && srcBmp.dib.bpp != 32 && srcBmp.dib.bpp != 8) || srcBmp.pImageData == NULL)
        return 0;
    //Gán các thông tin quan trọng
    desBmp.header = srcBmp.header;
    desBmp.dib = srcBmp.dib;
    //Cấp phát vùng nhớ cho phần dư (nếu có)
    if (desBmp.dib.dibSize > 40) {
        int remainder = desBmp.dib.dibSize - 40;
        desBmp.pDIBReserved = new char[remainder];
    }
    //Đảm bảo compression = 0
    desBmp.dib.compression = 0;
    //Cho kích thước CŨ = trị tuyệt đối để tính toán
    int oldHeight = (abs)(srcBmp.dib.height);
    int oldWidth = (abs)(srcBmp.dib.width);

    //Gán kích thước cho desBmp và floor (tránh trường hợp pixel lẻ)
    desBmp.dib.height = (floor)(srcBmp.dib.height / S);
    desBmp.dib.width = (floor)(srcBmp.dib.width / S);

    //Cho kích thước MỚI = trị tuyệt đối để tính toán
    int newHeight = (abs)(desBmp.dib.height);
    int newWidth = (abs)(desBmp.dib.width);

    //Tính byte trong width
    int nSrcByteInWidth = oldWidth * srcBmp.dib.bpp / 8;
    int nDesByteInWidth = newWidth * desBmp.dib.bpp / 8;

    //Tính padding bytes cho ảnh src và des theo công thức rút gọn
    int srcPadding = (4 - (nSrcByteInWidth) % 4) % 4;
    int desPadding = (4 - (nDesByteInWidth) % 4) % 4;
    //Tính imageSize
    desBmp.dib.imageSize = (newWidth * newHeight * desBmp.dib.bpp) / 8 + desPadding * newHeight;
    //Cấp phát vùng nhớ cho pixel data của ảnh desBmp
    desBmp.pImageData = new char[desBmp.dib.imageSize];

    //Con trỏ dòng trỏ đến pImageData
    char* pSrcRow = srcBmp.pImageData;
    char* pDesRow = desBmp.pImageData;

    //Số byte của 1 pixel ảnh
    int nSrcByteInPix = srcBmp.dib.bpp / 8;
    int nDesByteInPix = desBmp.dib.bpp / 8;

    //Số byte trên 1 dòng của ảnh
    int nSrcByteInRow = (oldWidth * nSrcByteInPix) + srcPadding;
    int nDesByteInRow = (newWidth * nDesByteInPix) + desPadding;
    //Trường hợp nếu resize ảnh 8bit ta cần tạo cho nó 1 bảng màu
    if (desBmp.dib.bpp <= 8) {
        for (int i = 0; i < 256; i++) {
            desBmp.colorTable[i].B = i;
            desBmp.colorTable[i].G = i;
            desBmp.colorTable[i].R = i;
            desBmp.colorTable[i].Reserved = 0;
        }
    }
    //Vòng lặp sẽ đi qua từng dòng và cột của ảnh Des
    for (int i = 0; i < newHeight; i++) {

        char* pPixSquare = pSrcRow;
        char* pDesPix = pDesRow;
        for (int j = 0; j < newWidth; j++) {
            //Xét trường hợp bpp = bao nhiêu để tính toán tương thích
            if (desBmp.dib.bpp == 8) {
                //Vì ảnh 8 bit chỉ có 1 giá trị màu nên ta chỉ gán lại 1 giá trị màu
                unsigned char ave = findAveColor(desBmp.dib, p, pPixSquare, nSrcByteInRow, nSrcByteInPix, S);
                pDesPix[0] = (char)ave;
            }

            if (desBmp.dib.bpp == 24) {
                //Ảnh 24 bit có 3 giá trị màu nên gán lại 3
                findAveColor(desBmp.dib, p, pPixSquare, nSrcByteInRow, nSrcByteInPix, S);
                pDesPix[0] = (char)p.B;
                pDesPix[1] = (char)p.G;
                pDesPix[2] = (char)p.R;
            }
            if (desBmp.dib.bpp == 32) {
                //Ảnh 32 bit có 4 giá trị màu nên gán lại 4
                findAveColor(desBmp.dib, p, pPixSquare, nSrcByteInRow, nSrcByteInPix, S);
                pDesPix[0] = (char)p.A;
                pDesPix[1] = (char)p.B;
                pDesPix[2] = (char)p.G;
                pDesPix[3] = (char)p.R;
            }
            //Ô vuông nhảy 1 đoạn số byte trong pixel * kích thước của ô vuông
            //Tránh trường hợp nhảy vào các ô đã tính toán
            pPixSquare += nSrcByteInPix * S;
            //pDesPix di chuyển sang pixel tiếp theo
            pDesPix += nDesByteInPix;
        }

        //Ô vuông nhảy 1 đoạn số byte trong dòng * kích thước ô vuông
        pSrcRow += nSrcByteInRow * S;
        //pDesRow di chuyển sang dòng tiếp theo
        pDesRow += nDesByteInRow;
    }
    return 1;

}

//Hàm in thông tin file
void printInfo(BMPImage srcImg) {
    //Xuất các thông tin đã đọc
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << "THONG TIN CUA FILE ANH" << endl;
    cout << "Chu ky: " << srcImg.header.signature << endl;
    cout << "Kich thuoc file: " << srcImg.header.fileSize << endl;
    cout << "Phan danh rieng: " << srcImg.header.reserved << endl;
    cout << "DataOffset: " << srcImg.header.dataOffset << endl;
    cout << "Kich thuoc DIB: " << srcImg.dib.dibSize << endl;
    cout << "Chieu dai x chieu rong: " << abs(srcImg.dib.height) << " x " << abs(srcImg.dib.width) << endl;
    cout << "So lop mau: " << srcImg.dib.planes << endl;
    cout << "So bit trong 1 pixel: " << srcImg.dib.bpp << endl;
    cout << "Compression: " << srcImg.dib.compression << endl;
    cout << "Image size: " << srcImg.dib.imageSize << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
}
//Hàm giải phóng bộ nhớ
void releaseMemory(BMPImage& srcImg, BMPImage& gray, BMPImage& zoom) {
    delete[] srcImg.pImageData, gray.pImageData, zoom.pImageData ;
    srcImg.pImageData = gray.pImageData = zoom.pImageData = NULL;
}



//Hàm giới thiệu thông tin
void introduction() {
    cout << "%%%%%%%%% BAI TAP LY THUYET 1 %%%%%%%%%" << endl;
    cout << "%%   Author: Nguyen Phuoc Anh Tuan   %%" << endl;
    cout << "%%           ID: 21120588            %%" << endl;
    cout << "%%%%%%%%% KY THUAT LAP TRINH %%%%%%%%%%" << endl;

}



int main(int argc, char* argv[]) {
    BMPImage srcImg;
    BMPImage grayImg;
    BMPImage zoomImg;
    int S = 0;

    //Giới thiệu bài tập
    introduction();
    //Nếu số lượng tham số ít hơn hoặc vượt quá yêu cầu, quay lai
    if (argc > 5 || argc < 4) {
        cout << "So luong tham so khong hop le\n";
        cout << "Vui long thu lai\n";
        return 0;
    }
    //Thực hiện chuyển 8 bit
    if (argc == 4) {
        if (_strcmpi(argv[1], "-conv") != 0) {
            cout << "Dong lenh chua dung!\n";
            return 0;
        }
        else {
            readBMP(argv[2], srcImg);
            cout << "Thong tin anh SOURCE\n";
            printInfo(srcImg);
            to8Bit(srcImg, grayImg);
            writeBMP(argv[3], grayImg);
            printInfo(grayImg);
            cout << "Chuyen 8 bit thanh cong!\n";
        }

    }
    //Thực hiện zoom
    if (argc == 5) {
        if (_strcmpi(argv[1], "-zoom") != 0) {
            cout << "Dong lenh chua dung!\n";
            return 0;
        }
        else {
            S = atoi(argv[4]);
            readBMP(argv[2], srcImg);
            cout << "Thong tin anh SOURCE\n";
            printInfo(srcImg);
            zoomIn(srcImg, zoomImg, S);
            writeBMP(argv[3], zoomImg);
            printInfo(zoomImg);
            cout << "Thu nho anh thanh cong!\n";
        }

    }
 
    releaseMemory(srcImg, grayImg, zoomImg);

    return 1;



}