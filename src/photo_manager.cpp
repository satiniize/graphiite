
// Technically very inefficient, not sure
int get_selected_photos_count() {
  int count = 0;
  for (auto photo : photos) {
    if (photo.selected) {
      count++;
    }
  }
  return count;
}

bool seperate_photos(std::vector<Photo> &photos) {
  std::filesystem::path root_path(photos[0].file_path.parent_path());

  std::filesystem::create_directories(root_path / "Curated");
  std::filesystem::create_directories(root_path / "Discarded");

  for (auto &photo : photos) {
    if (photo.selected) {
      std::filesystem::copy_file(
          photo.file_path, root_path / "Curated" / photo.file_path.filename(),
          std::filesystem::copy_options::overwrite_existing);
    } else {
      std::filesystem::copy_file(
          photo.file_path, root_path / "Discarded" / photo.file_path.filename(),
          std::filesystem::copy_options::overwrite_existing);
    }
    std::filesystem::remove(photo.file_path);
  }
  return true;
}

bool load_photos(std::filesystem::path path) {
  if (!std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
    SDL_Log("Invalid photo path");
    return 1;
  }

  photos.clear();

  for (const std::filesystem::directory_entry &entry :
       std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      SDL_Log("Loading file: %s", entry.path().c_str());
      // std::cout << "Loading file: " << entry.path() << std::endl;

      ImageData photo_image_data{};
      photo_image_data.path = entry.path().string();
      photo_image_data.tiling = false;

      // UI specific struct
      Photo photo{};
      photo.image_data = photo_image_data;
      photo.selected = false;
      photo.file_path = entry.path();

      photos.push_back(photo);

      std::ifstream jpegStream(entry.path());
      if (!jpegStream.is_open()) {
        SDL_Log("ERROR: opening input file %s: %s", entry.path().c_str(),
                strerror(errno));
      }

      jpegStream.seekg(0, std::ios::end);
      std::streampos size = jpegStream.tellg();
      if (size == 0) {
        SDL_Log("WARNING: Input file contains no data");
      }
      jpegStream.seekg(0, std::ios::beg);
      size_t jpegSize = static_cast<size_t>(size);

      unsigned char *jpegBuf = (unsigned char *)malloc(jpegSize);
      if (!jpegBuf) {
        SDL_Log("ERROR: allocating JPEG buffer");
      }

      jpegStream.read(reinterpret_cast<char *>(jpegBuf), jpegSize);
      jpegStream.close();

      // 2. Initialize TurboJPEG decompressor
      tjhandle tjInstance = tj3Init(TJINIT_DECOMPRESS);
      if (!tjInstance) {
        SDL_Log("ERROR: creating TurboJPEG instance");
      }

      // 3. Read JPEG header to get image info
      int jpegWidth, jpegHeight, jpegSubsamp, jpegColorspace, jpegPrecision;
      if (tj3DecompressHeader(tjInstance, jpegBuf, jpegSize) < 0) {
        SDL_Log("ERROR: reading JPEG header for %s: %s", entry.path().c_str(),
                tj3GetErrorStr(tjInstance));
      }

      jpegWidth = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
      jpegHeight = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
      jpegSubsamp = tj3Get(tjInstance, TJPARAM_SUBSAMP);
      jpegColorspace = tj3Get(tjInstance, TJPARAM_COLORSPACE);
      jpegPrecision = tj3Get(tjInstance, TJPARAM_PRECISION);

      // --- Prepare for Decompression and SDL Surface Creation ---
      // We'll target 8-bit per channel output for SDL_Surface compatibility.
      // If the JPEG is higher precision, we'll convert it.

      // For general compatibility, decompres to BGRX (32-bit per pixel)
      // For some reason, the backwards thing here happens again
      int tjDecompressFormat = TJPF_RGBA;
      SDL_PixelFormat sdlPixelFormat = SDL_PIXELFORMAT_ABGR8888;
      int sdlPixelSize =
          tjPixelSize[tjDecompressFormat]; // Will be 4 bytes for TJPF_BGRX

      // Check for higher precision JPEGs
      unsigned char *decompressedBuf_8bit =
          NULL; // This will hold the final 8-bit data for SDL
      void *tjOutputRawBuf = NULL; // Intermediate buffer if precision > 8
      int tjOutputRawBufSize = 0;

      // 8 Bit
      if (jpegPrecision <= 8) {
        tjOutputRawBufSize =
            jpegWidth * jpegHeight * sdlPixelSize; // Size for 8-bit data
        tjOutputRawBuf = malloc(tjOutputRawBufSize);
        if (!tjOutputRawBuf) {
          SDL_Log("ERROR: allocating 8-bit TurboJPEG output buffer for %s: %s",
                  entry.path().c_str(), strerror(errno));
          // goto cleanup_loop;
        }
        if (tj3Decompress8(tjInstance, jpegBuf, jpegSize,
                           (unsigned char *)tjOutputRawBuf, 0,
                           tjDecompressFormat) < 0) {
          SDL_Log("ERROR: decompressing 8-bit JPEG image %s: %s",
                  entry.path().c_str(), tj3GetErrorStr(tjInstance));
          free(tjOutputRawBuf); // Decompression failed, free buffer
          // goto cleanup_loop;
        }
        decompressedBuf_8bit =
            (unsigned char *)tjOutputRawBuf; // Direct use for 8-bit
        // SDL will take ownership of decompressedBuf_8bit later, so don't free
        // it here
        tjOutputRawBuf =
            NULL; // Clear pointer to prevent double free via cleanup_loop
      } else {    // Handle 12 or 16-bit JPEGs, convert to 8-bit for SDL
        SDL_Log("WARNING: JPEG %s has precision %d > 8 bits. Converting to "
                "8-bit per channel.",
                entry.path().c_str(), jpegPrecision);

        // TurboJPEG outputs unsigned short for precision > 8
        int tjIntermediatePixelSize_16bit =
            tjPixelSize[tjDecompressFormat] *
            sizeof(unsigned short); // e.g., 4 channels * 2 bytes/channel = 8
                                    // bytes/pixel
        tjOutputRawBufSize =
            jpegWidth * jpegHeight * tjIntermediatePixelSize_16bit;
        tjOutputRawBuf = malloc(tjOutputRawBufSize);
        if (!tjOutputRawBuf) {
          SDL_Log("ERROR: allocating 16-bit TurboJPEG intermediate buffer for "
                  "%s: %s",
                  entry.path().c_str(), strerror(errno));
          // goto cleanup_loop;
        }

        // 12 Bit
        if (jpegPrecision <= 12) {
          if (tj3Decompress12(tjInstance, jpegBuf, jpegSize,
                              (short int *)tjOutputRawBuf, 0,
                              tjDecompressFormat) < 0) {
            SDL_Log("ERROR: decompressing 12-bit JPEG image %s: %s",
                    entry.path().c_str(), tj3GetErrorStr(tjInstance));
            free(tjOutputRawBuf);
            // goto cleanup_loop;
          }
          // 12 Bit
        } else { // Assume precision <= 16
          if (tj3Decompress16(tjInstance, jpegBuf, jpegSize,
                              (unsigned short *)tjOutputRawBuf, 0,
                              tjDecompressFormat) < 0) {
            SDL_Log("ERROR: decompressing 16-bit JPEG image %s: %s",
                    entry.path().c_str(), tj3GetErrorStr(tjInstance));
            free(tjOutputRawBuf);
            // goto cleanup_loop;
          }
        }

        // Now, convert the 16-bit (unsigned short) tjOutputRawBuf to 8-bit
        // (unsigned char) decompressedBuf_8bit
        decompressedBuf_8bit = (unsigned char *)malloc(
            jpegWidth * jpegHeight * sdlPixelSize); // Final 8-bit size
        if (!decompressedBuf_8bit) {
          SDL_Log("ERROR: allocating 8-bit conversion buffer for %s: %s",
                  entry.path().c_str(), strerror(errno));
          free(tjOutputRawBuf);
          // goto cleanup_loop;
        }

        unsigned short *src_ptr = (unsigned short *)tjOutputRawBuf;
        unsigned char *dst_ptr = decompressedBuf_8bit;

        // Loop through pixels and convert
        for (int i = 0; i < jpegWidth * jpegHeight; ++i) {
          // Assuming TJPF_BGRX output (4 channels per pixel)
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // B
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // G
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // R
          *dst_ptr++ = (unsigned char)0xFF; // X/Alpha (fully opaque)
        }
        free(tjOutputRawBuf); // Free the intermediate 16-bit buffer
        tjOutputRawBuf = NULL;
      }

      // 4. Create an SDL_Surface from the decompressed 8-bit data
      // Using the new SDL3 function: SDL_CreateSurfaceFrom
      SDL_Surface *original_image_surface = SDL_CreateSurfaceFrom(
          jpegWidth,      // Width
          jpegHeight,     // Height
          sdlPixelFormat, // SDL Pixel Format (e.g., SDL_PIXELFORMAT_BGRX8888)
          decompressedBuf_8bit,    // Pointer to existing pixel data (8-bit)
          jpegWidth * sdlPixelSize // Pitch (bytes per row)
      );

      // Important: According to SDL3 wiki for SDL_CreateSurfaceFrom,
      // "Pixel data is not managed automatically; you must free the surface
      // before you free the pixel data."
      // This means SDL *does NOT* take ownership of decompressedBuf_8bit,
      // and we MUST free it ourselves *after* Destroying
      // original_image_surface. This is a crucial change from SDL2's
      // SDL_CreateRGBSurfaceWithFormatFrom!

      if (original_image_surface == NULL) {
        SDL_Log("ERROR: Failed to create SDL_Surface from decompressed data "
                "for %s: %s",
                entry.path().c_str(), SDL_GetError());
        free(decompressedBuf_8bit); // SDL didn't take ownership, so we must
                                    // free.
        // goto cleanup_loop;
      }

      // 5. Downsample the image
      int downsample_factor = 10;
      int thumbWidth = original_image_surface->w / downsample_factor;
      int thumbHeight = original_image_surface->h / downsample_factor;
      if (thumbWidth == 0)
        thumbWidth = 1; // Ensure minimum 1 pixel
      if (thumbHeight == 0)
        thumbHeight = 1;

      SDL_Surface *downsampled = SDL_CreateSurface(
          thumbWidth, thumbHeight,
          original_image_surface
              ->format); // SDL_CreateSurface (new in SDL3 for simpler creation)

      if (downsampled == NULL) {
        SDL_Log("ERROR: Failed to create downsampled SDL_Surface for %s: %s",
                entry.path().c_str(), SDL_GetError());
        SDL_DestroySurface(original_image_surface);
        free(decompressedBuf_8bit); // Must free this explicitly!
        // goto cleanup_loop;
      }

      SDL_Rect src_rect = {0, 0, original_image_surface->w,
                           original_image_surface->h};
      SDL_Rect dst_rect = {0, 0, thumbWidth, thumbHeight};

      if (!SDL_BlitSurfaceScaled(original_image_surface, &src_rect, downsampled,
                                 &dst_rect, SDL_SCALEMODE_LINEAR)) {
        SDL_Log("ERROR: Failed to scale surface for %s: %s",
                entry.path().c_str(), SDL_GetError());
        SDL_DestroySurface(original_image_surface);
        free(decompressedBuf_8bit); // Must free this explicitly!
        SDL_DestroySurface(downsampled);
        // goto cleanup_loop;
      }

      // 6. Load texture into your renderer
      renderer.load_texture(entry.path().string(), downsampled);

      // 7. Clean up surfaces and TurboJPEG instance
      SDL_DestroySurface(original_image_surface);
      free(decompressedBuf_8bit); // CRITICAL: Free the buffer that
                                  // original_image_surface pointed to
      SDL_DestroySurface(downsampled);
      free(jpegBuf);
      tj3Destroy(tjInstance);
    }
  }

  return true;
}
