# BlurringFilter
 A command line mini program that blurs an image

USAGE: BlurringFilter -f <factor> -i <input-file-path> -o <output-file-path>

This program blurs a TARGA24/TARGA32 (true color without run-lenght encoding) image from 
a factor of 0 (no blur) to a factor of 1 (kernel size = min(image_height, img_width) / 2).

Internally it uses the box blur algorithm with separated filter and running average
optimization. I have also implemented an almost working summed area table optimization, 
that I have left commented out considering the vast amount of time I have wasted on it :)

Bonus1: To increase the blur quality, the image gets reflect padded along the edges before 
filtering.

Bonus2: The program is able to properly interpret images that are flipped vertically or
horizontally, using the bits 4&5 of the image descriptor field in the header.

Some of the information sources I've used during the research phase are listed here:
- https://www.scratchapixel.com/lessons/digital-imaging/simple-image-manipulations
- https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
- https://unix4lyfe.org/targa/
- https://www.gamasutra.com/view/feature/131511/four_tricks_for_fast_blurring_in_.php
- http://amritamaz.net/blog/understanding-box-blur

Compiler version used: MSVC++ 14.16.