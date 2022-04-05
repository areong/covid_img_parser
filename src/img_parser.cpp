#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

struct Vector2 {
    Vector2(int x, int y): x(x), y(y) {}
    int x;
    int y;
};

Vector2 operator+(const Vector2& a, const Vector2& b) {
    return Vector2(a.x + b.x, a.y + b.y);
}
Vector2 operator-(const Vector2& a, const Vector2& b) {
    return Vector2(a.x - b.x, a.y - b.y);
}
Vector2 operator*(float value, const Vector2& a) {
    return Vector2((int) (value * a.x), (int) (value * a.y));
}
Vector2 operator*(const Vector2& a, float value) {
    return Vector2((int) (value * a.x), (int) (value * a.y));
}

struct BoundingBox {
    BoundingBox(): min(1e8, 1e8), max(-1e8, -1e8) {}
    Vector2 min;
    Vector2 max;
    // bool overlap_x(const BoundingBox& other) const {
    //     if (other.min.x < min.x) {
    //         return other.max.x > min.x;
    //     } else {
    //         return other.min.x < max.x;
    //     }
    // }
    // bool overlap_y(const BoundingBox& other) const {
    //     if (other.min.y < min.y) {
    //         return other.max.y > min.y;
    //     } else {
    //         return other.min.y < max.y;
    //     }
    // }
    int width() const {
        return max.x - min.x;
    }
    int height() const {
        return max.y - min.y;
    }
    void grow(const Vector2& point) {
        if (point.x < min.x) min.x = point.x;
        if (point.x > max.x) max.x = point.x;
        if (point.y < min.y) min.y = point.y;
        if (point.y > max.y) max.y = point.y;
    }
    void grow(const BoundingBox& other) {
        if (other.min.x < min.x) min.x = other.min.x;
        if (other.min.y < min.y) min.y = other.min.y;
        if (other.max.x > max.x) max.x = other.max.x;
        if (other.max.y > max.y) max.y = other.max.y;
    }
    void shrink(float ratio) {
        auto center = 0.5f * (min + max);
        min = ratio * min + (1 - ratio) * center;
        max = ratio * max + (1 - ratio) * center;
    }
    void reset() {
        min = Vector2(1e8, 1e8);
        max = Vector2(-1e8, -1e8);
    }
};

struct Paragraph {
    BoundingBox bb;
    std::string text;
    void reset() {
        bb.reset();
        text = "";
    }
};

bool less_min_x(const Paragraph& a, const Paragraph& b) {
    return a.bb.min.x < b.bb.min.x;
}

bool less_min_y(const Paragraph& a, const Paragraph& b) {
    return a.bb.min.y < b.bb.min.y;
}

bool overlap_x(const BoundingBox& a, const BoundingBox& b) {
    if (b.min.x < a.min.x) {
        return b.max.x > a.min.x;
    } else {
        return b.min.x < a.max.x;
    }
}

bool overlap_y(const BoundingBox& a, const BoundingBox& b) {
    if (b.min.y < a.min.y) {
        return b.max.y > a.min.y;
    } else {
        return b.min.y < a.max.y;
    }
}

class ParagraphGroup {
private:
    std::vector<Paragraph> paragraphs;
    BoundingBox bb;

public:
    const std::vector<Paragraph>& get_paragraphs() const { return paragraphs; };
    const BoundingBox& get_bb() const { return bb; };
    size_t empty() const {
        return paragraphs.empty();
    }
    void push_back(const Paragraph& paragraph) {
        // Insert.
        paragraphs.push_back(paragraph);
        // Update bounding box.
        bb.grow(paragraph.bb);
    }
    void push_back(Paragraph&& paragraph) {
        // Insert.
        paragraphs.push_back(paragraph);
        // Update bounding box.
        bb.grow(paragraph.bb);
    }
    void sort(bool sort_min_x) {
        auto less = sort_min_x ? less_min_x : less_min_y;
        std::sort(paragraphs.begin(), paragraphs.end(), less);
    }
    bool contain(const std::string& text) const {
        for (auto&& paragraph : paragraphs) {
            if (paragraph.text.find(text) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

std::vector<ParagraphGroup> split(const ParagraphGroup& in_group, bool split_x) {
    std::vector<ParagraphGroup> out_groups;

    // Choose the comparator for sorting.
    auto less = split_x ? less_min_x : less_min_y;

    // Choose the overlap checking function.
    auto overlap = split_x ? overlap_x : overlap_y;

    // Copy paragraphs and sort.
    auto in_paragraphs = in_group.get_paragraphs();
    std::sort(in_paragraphs.begin(), in_paragraphs.end(), less);

    // Create the first group.
    out_groups.emplace_back();
    // Check each paragraph.
    for (auto&& paragraph : in_paragraphs) {
        // If the current group is empty, add the paragraph to the group.
        auto& out_group = out_groups.back();
        if (out_group.empty()) {
            out_group.push_back(paragraph);
            continue;
        }

        // The group is not empty. Check if the paragraph overlap the group.
        if (overlap(out_group.get_bb(), paragraph.bb)) {
            // Overlap. Add to the group.
            out_group.push_back(paragraph);
        } else {
            // Not overlap. Add to a new group.
            out_groups.emplace_back();
            out_groups.back().push_back(paragraph);
        }
    }

    return out_groups;
}

ParagraphGroup merge(const std::vector<ParagraphGroup>& in_groups,
    int begin_index, int end_index) {
    // Output container.
    ParagraphGroup out_group;

    // Check index;
    if (begin_index < 0 || end_index <= 0 ||
        begin_index >= in_groups.size() || end_index > in_groups.size()) {
        std::cerr << "ERROR in merge(): invalid index\n";
        return out_group;
    }

    // Add paragraphs to output.
    for (int i = begin_index; i < end_index; ++i) {
        auto& in_group = in_groups[i];
        for (auto&& paragraph : in_group.get_paragraphs()) {
            out_group.push_back(paragraph);
        }
    }

    // Output.
    return out_group;
}

ParagraphGroup read_paragraphs(const nlohmann::json& vision_result,
    const std::string& image_filename) {

    cv::Mat image = cv::imread(image_filename);
    // // Draw bounding box of paragraphs.
    // for (auto&& paragraph : paragraph_group.get_paragraphs()) {
    //     auto& min = paragraph.bb.min;
    //     auto& max = paragraph.bb.max;
    //     cv::rectangle(image, cv::Point(min.x, min.y), cv::Point(max.x, max.y), cv::Scalar(0, 100, 0));
    // }

    // cv::imshow("image", image);
    // cv::waitKey(0);

    ParagraphGroup out_group;

    auto& fullTextAnnotation = vision_result["fullTextAnnotation"];
    for (auto&& page : fullTextAnnotation["pages"]) {
        for (auto&& block : page["blocks"]) {
            for (auto&& paragraph : block["paragraphs"]) {
                // Create a new output paragraph.
                Paragraph out_paragraph;

                // std::string current_text;

                // Store the text.
                for (auto&& word : paragraph["words"]) {
                    for (auto&& symbol : word["symbols"]) {
                        // If the symbol does not contain text, skip.
                        if (!symbol.contains("text")) continue;

                        // Read the bounding box of the symbol.
                        BoundingBox symbol_bb;
                        for (auto&& vertex : symbol["boundingBox"]["vertices"]) {
                            Vector2 out_vertex(vertex["x"], vertex["y"]);
                            symbol_bb.grow(out_vertex);
                        }

                        // // Draw bb.
                        // {
                        //     auto& min = symbol_bb.min;
                        //     auto& max = symbol_bb.max;
                        //     cv::rectangle(image, cv::Point(min.x, min.y), cv::Point(max.x, max.y), cv::Scalar(200, 0, 0));
                        //     cv::imshow("image", image);
                        //     cv::waitKey(0);
                        // }

                        // Shrink the bounding box to decrease the chance of overlapping.
                        // symbol_bb.shrink(0.5f);

                        // Check if the symbol is located at a different line
                        // by checking if the symbol's bounding box overlaps with
                        // the current paragraph bounding box in y direction.
                        // // If on the same line, check if the current symbol is
                        // // located too right to the bounding box of the current paragraph.
                        if (!out_paragraph.text.empty() &&
                            !overlap_y(out_paragraph.bb, symbol_bb)) {
                            // (!overlap_y(out_paragraph.bb, symbol_bb) ||
                            // ((symbol_bb.min.x - out_paragraph.bb.max.x) > symbol_bb.width()))) {
                            // Prepare to start a new paragraph.
                            // Store the current paragraph first (by copy).
                            // std::cout << out_paragraph.text << "\n";

                            // // Draw bb.
                            // {
                            //     auto& min = out_paragraph.bb.min;
                            //     auto& max = out_paragraph.bb.max;
                            //     cv::rectangle(image, cv::Point(min.x, min.y), cv::Point(max.x, max.y), cv::Scalar(0, 0, 200));
                            //     cv::imshow("image", image);
                            //     cv::waitKey(0);
                            // }

                            out_group.push_back(out_paragraph);

                            // Reset the current paragraph to serve as a new paragraph.
                            out_paragraph.reset();
                        }

                        // Collect text.
                        out_paragraph.text += symbol["text"];
                        // current_text += symbol["text"];

                        // Collect bounding box.
                        out_paragraph.bb.grow(symbol_bb);

                        // for (auto&& vertex : symbol["boundingBox"]["vertices"]) {
                        //     Vector2 out_vertex(vertex["x"], vertex["y"]);
                        //     out_paragraph.bb.grow(out_vertex);
                        // }

                        // // Check line break.
                        // if (symbol.contains("property") && symbol["property"].contains("detectedBreak")) {
                        //     std::cout << current_text << "(line break)\n";
                        //     current_text = "";
                        // }
                    }
                }

                // If a symbol has a member, symbol["property"]["detectedBreak"],
                // it implies that a line break detected.
                // Split the current paragraph at the line break.
                // Do not store the current word.
                // Put the words afterwards to a new paragraph.

                // Note: do not use detected break:
                // symbol["property"].contains("detectedBreak")
                // because a break can be incorrectly detected at the middle of a line.

                // Store the bounding box.
                // for (auto&& vertex : paragraph["boundingBox"]["vertices"]) {
                //     Vector2 out_vertex(vertex["x"], vertex["y"]);
                //     out_paragraph.bb.grow(out_vertex);
                // }

                // // Draw bb.
                // {
                //     auto& min = out_paragraph.bb.min;
                //     auto& max = out_paragraph.bb.max;
                //     cv::rectangle(image, cv::Point(min.x, min.y), cv::Point(max.x, max.y), cv::Scalar(0, 0, 200));
                //     cv::imshow("image", image);
                //     cv::waitKey(0);
                // }

                // Store the paragraph for output.
                out_group.push_back(out_paragraph);
            }
        }
    }

    return out_group;
}

// Split into y-non-overlapping rows.
std::vector<std::vector<Paragraph>> split_paragraphs_into_rows() {
    std::vector<std::vector<Paragraph>> rows;
    return rows;
}

void print(const ParagraphGroup& group) {
    for (auto&& paragraph : group.get_paragraphs()) {
        std::cout << paragraph.text << "\n";
        // std::cout << "bb: " << paragraph.bb.min.x << ", " << paragraph.bb.min.y << ", "
        //     << paragraph.bb.max.x << ", " << paragraph.bb.max.y << "\n";
    }
}

void show_image(const std::string& filename, const ParagraphGroup& paragraph_group) {
    cv::Mat image = cv::imread(filename);

    // Draw bounding box of paragraphs.
    for (auto&& paragraph : paragraph_group.get_paragraphs()) {
        auto& min = paragraph.bb.min;
        auto& max = paragraph.bb.max;
        cv::rectangle(image, cv::Point(min.x, min.y), cv::Point(max.x, max.y), cv::Scalar(0, 100, 0));
    }

    cv::imshow("image", image);
    cv::waitKey(0);
}

int main(int argc, char** argv) {

    // // Test
    // Vector2 a(2, 2);
    // Vector2 b(4, -4);
    // auto c = a + b;
    // auto d = a * 0.5f;
    // auto e = 1.5f * b;
    // std::cout << "c: (" << c.x << ", " << c.y << ")\n";
    // std::cout << "d: (" << d.x << ", " << d.y << ")\n";
    // std::cout << "e: (" << e.x << ", " << e.y << ")\n";

    // Parse input argument.
    std::string image_filename;
    std::string json_filename;
    bool row_range_is_specified_by_input = false;
    int input_row_begin_index = 0;
    int input_row_end_index = 0;
    if (argc == 2) {
        json_filename = argv[1];
    } else if (argc == 4) {
        json_filename = argv[1];
        row_range_is_specified_by_input = true;
        input_row_begin_index = std::atoi(argv[2]);
        input_row_end_index = std::atoi(argv[3]);
        if (input_row_begin_index < 0 || input_row_end_index < 0 ||
            input_row_begin_index > input_row_end_index) {
            std::cout << "Invalid input indices\n";
            return 0;
        }
    } else {
        std::cout << "Invalid input\n";
        return 0;
    }


    std::ifstream json_file(json_filename);
    nlohmann::json vision_result;
    json_file >> vision_result;

    // std::cout << "json_file.size(): " << vision_result.size() << "\n";
    // std::cout << vision_result["fullTextAnnotation"]["text"] << "\n";

    auto paragraph_group = read_paragraphs(vision_result, image_filename);

    // std::cout << "read paragraphs:\n";
    // print(paragraph_group);
    // std::cout << "read completed\n\n";
    // std::cout << "paragraph count: " << paragraph_group.get_paragraphs().size() << "\n";

    // // Sort by bb min y.
    // paragraph_group.sort_min_y();

    // Find table by brute force.

    // Split into rows.
    auto paragraph_rows = split(paragraph_group, false);

    std::cout << "\nRow count: " << paragraph_rows.size() << "\n\n";
    for (int i = 0; i < paragraph_rows.size(); ++i) {
        std::cout << "Row " << i << ":\n\n";
        print(paragraph_rows[i]);
        std::cout << "\n";
    }

    // Find a subset of rows with the largest column count.
    // Two variables, row_begin_index and row_end_index.
    // row_begin_index range: [0, total_row_count)
    // row_end_index range: [row_begin_index + 1, total_row_count]
    const auto total_row_count = paragraph_rows.size();
    std::vector<std::vector<int>> column_count_table(total_row_count,
        std::vector<int>(total_row_count + 1, 0));
    
    // Calculate column count for each begin, end pair in the table.
    // Also record the maximum column count.
    int max_column_count = 0;
    for (int row_begin_index = 0; row_begin_index < total_row_count; ++row_begin_index) {
        for (int row_end_index = row_begin_index + 1; row_end_index < total_row_count + 1; ++row_end_index) {
            // Merge rows between the begin and end indices.
            auto merged_group = merge(paragraph_rows, row_begin_index, row_end_index);

            // Split the merged group into columns.
            auto paragraph_columns = split(merged_group, true);

            // Record the column count.
            auto column_count = paragraph_columns.size();
            column_count_table[row_begin_index][row_end_index] = column_count;
            // // Print the result.
            // std::cout << "(" << row_begin_index << ", " << row_end_index << "): "
            //     << column_count_table[row_begin_index][row_end_index] << "\n";

            // Update the maximum column count.
            if (column_count > max_column_count) {
                max_column_count = column_count;
            }
        }
    }

    // In the index pairs with the maximum column count, find the pair with
    // the smallest row_begin_index and the largest row_end_index, and
    // with the most rows.
    // Method: iterate the begin index ascendingly, and the end index descendingly,
    // and stop at the first element with the maximum column count.
    // std::cout << "total_row_count: " << total_row_count << "\n";
    // std::cout << "max_column_count: " << max_column_count << "\n";
    // int best_row_begin_index = 0;
    // int best_row_end_index = total_row_count;
    std::vector<std::pair<int, int>> row_range_candidates;
    // bool best_is_found = false;
    for (int row_begin_index = 0; row_begin_index < total_row_count; ++row_begin_index) {
        for (int row_end_index = total_row_count; row_end_index > row_begin_index; --row_end_index) {
            // std::cout << "(" << row_begin_index << ", " << row_end_index << "): "
            //     << column_count_table[row_begin_index][row_end_index] << "\n";
            if (column_count_table[row_begin_index][row_end_index] == max_column_count) {
                // Collect the row range as a candidate.
                row_range_candidates.push_back(std::make_pair(row_begin_index, row_end_index));

                // best_row_begin_index = row_begin_index;
                // best_row_end_index = row_end_index;
                // best_is_found = true;
                // break;
            }
        }

        // if (best_is_found) break;
    }
    // std::cout << "best: (" << best_row_begin_index << ", " << best_row_end_index << ")\n";

    // Sort candidates by row count.
    std::sort(row_range_candidates.begin(), row_range_candidates.end(),
        [](const std::pair<int, int>& a, const std::pair<int, int>& b){
            return (a.second - a.first) < (b.second - b.first);
        });

    // // Print candidates.
    // std::cout << "row range candidates:\n";
    // for (auto&& range : row_range_candidates) {
    //     std::cout << "(" << range.first << ", " << range.second << ")\n";
    // }

    // Extract best columns.
    {
        auto best_row_range = row_range_candidates.back();
        int best_row_begin_index = best_row_range.first;
        int best_row_end_index = best_row_range.second;

        // If the input specifies the row range, overwrite the range here.
        if (row_range_is_specified_by_input) {
            best_row_begin_index = input_row_begin_index;
            best_row_end_index = input_row_end_index;
        }

        std::cout << "Split rows in range [" << best_row_begin_index << ", " << best_row_end_index << ") into columns.\n";

        // Merge rows between the begin and end indices.
        auto merged_group = merge(paragraph_rows, best_row_begin_index, best_row_end_index);
        // std::cout << "merged_group size: " << merged_group.get_paragraphs().size() << "\n";

        // Split the merged group into columns.
        auto paragraph_columns = split(merged_group, true);
        // std::cout << "column count: " << paragraph_columns.size() << "\n";

        // Sort each column by y.
        for (auto&& column : paragraph_columns) {
            column.sort(false);
        }

        // Print columns.
        std::cout << "\nColumn count: " << paragraph_columns.size() << "\n\n";
        for (int i = 0; i < paragraph_columns.size(); ++i) {
            std::cout << "Column " << i << ":\n\n";
            print(paragraph_columns[i]);
            std::cout << "\n";
        }
    }

    // // Find the row of column titles.
    // // Assume the titles contain "日期", "時間", or "地點".
    // auto row_begin_index = 0;
    // for (int i = 0; i < paragraph_rows.size(); ++i) {
    //     std::cout << "row " << i << ":\n";
    //     auto& row = paragraph_rows[i];
    //     // print(row);

    //     if (row.contain("日期") || row.contain("時間") || row.contain("地點")) {
    //         // std::cout << "  contain 日期\n";
    //         row_begin_index = i;
    //         break;
    //     }
    // }

    // Find the row containing "data", "time", or "location" (in Chinese).

    // show_image(image_filename, paragraph_group);

    return 0;
}
