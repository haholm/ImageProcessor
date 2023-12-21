from scipy import signal
import numpy as np
from PIL import Image
import argparse
import os
import math

def main(filename: str):
    print(filename)
    if not os.path.isabs(filename):
        full_path = os.path.join(os.getcwd(), filename)
        if not os.path.exists(full_path):
            raise f"Could not find the provided file {filename} from {os.getcwd()}. Please provide the full path."
        
        filename = full_path
            
    with Image.open(filename) as im:
        im_arr = np.asarray(im)
        im_arr_r = im_arr[:,:,0]
        im_arr_g = im_arr[:,:,1]
        im_arr_b = im_arr[:,:,2]
        
        filter_size = 40
        lp1d_filter_list = [2**(math.log2(filter_size)-abs(x)) for x in range(-filter_size // 2, filter_size // 2)]
        lp1d_filter = np.array([lp1d_filter_list]) / sum(lp1d_filter_list)
        lp2d_filter = signal.convolve2d(lp1d_filter, lp1d_filter.T)
        
        filtered_r = signal.convolve2d(im_arr_r, lp2d_filter, mode="same")
        filtered_g = signal.convolve2d(im_arr_g, lp2d_filter, mode="same")
        filtered_b = signal.convolve2d(im_arr_b, lp2d_filter, mode="same")
        filtered_im = np.array([filtered_r, filtered_g, filtered_b]).transpose(1, 2, 0)
        
        Image.fromarray(filtered_im.astype(np.uint8)).show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                prog=os.path.basename(__file__),
                description='Processes images')
    parser.add_argument("filename")
    parser.add_argument("--heif", help="enable heif support, requires pillow_heif package", action="store_const", default=False, const=True);
    args = parser.parse_args()
    if (args.heif):
        from pillow_heif import register_heif_opener
        register_heif_opener()
        
    main(args.filename)