#include "photo_manager.hpp"

// int get_selected_photos_count() {
//   int count = 0;
//   for (auto photo : photos) {
//     if (photo.selected) {
//       count++;
//     }
//   }
//   return count;
// }

// bool seperate_photos(std::vector<Photo> &photos) {
//   std::filesystem::path root_path(photos[0].file_path.parent_path());

//   std::filesystem::create_directories(root_path / "Curated");
//   std::filesystem::create_directories(root_path / "Discarded");

//   for (auto &photo : photos) {
//     if (photo.selected) {
//       std::filesystem::copy_file(
//           photo.file_path, root_path / "Curated" /
//           photo.file_path.filename(),
//           std::filesystem::copy_options::overwrite_existing);
//     } else {
//       std::filesystem::copy_file(
//           photo.file_path, root_path / "Discarded" /
//           photo.file_path.filename(),
//           std::filesystem::copy_options::overwrite_existing);
//     }
//     std::filesystem::remove(photo.file_path);
//   }
//   return true;
// }
