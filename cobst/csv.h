#pragma once

#ifndef __CSV_H
#define __CSV_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm>

/*    2019-05-30    Bob Dubner

    I perceived a need for a generalized generator of .CSV files

    */

class CSV_GENERATOR
{
private:
    class CSV_COLUMN
    {
    public:
        int id;
        std::string header;
        std::vector<std::string> data;

    public:
        CSV_COLUMN(int const &id_,std::string const &header_) : id(id_), header(header_) {}
    };

private:
    std::vector<CSV_COLUMN> columns;
    std::unordered_map<int,size_t> id_to_column;

    std::string Linify(const std::string &s)
    {
#if 0 // This code was for quotes and commas
        std::string retval = "\"";
        for(char ch : s) {
            retval += ch;
            if(ch == '\"') {
                retval += ch;
            }
        }
        retval += '\"';
#endif
        return s;
    }

    bool IsConsistent()
    {
        bool okay = true;
        size_t rows = 0;
        const size_t ncolumns = columns.size();
        if(ncolumns > 0) {
            rows = columns[0].data.size();
        }
        for(size_t i=1; i<ncolumns; i++) {
            if(columns[i].data.size() != rows) {
                okay = false;
                break;
            }
        }
        if(!okay) {
            std::cerr << "The number of rows in each column is inconsistent:" << std::endl;
            for(size_t i=0; i<ncolumns; i++) {
                std::cerr << columns[i].id << " " << columns[i].header << " " << columns[i].data.size() << std::endl;
            }
            exit(1);
        }
        return okay;
    }

public:
    void AddColumn(int id, std::string header)
    {
        if(id_to_column.find(id) != id_to_column.end()) {
            std::cerr << "You are trying to reuse a column identifier in CSV_GENERATOR::AddColumn().\n";
            std::cerr << "ID: " << id << " is already in use for \"" << columns[id_to_column[id]].header << "\"" << std::endl;
            std::cerr << "and you are trying to use that same ID for \"" << header << "\"." << std::endl;
            exit(1);
        }
        columns.push_back(CSV_COLUMN(id,header));
        id_to_column[id] = columns.size()-1;
    }

    void AddData(int id, std::string data)
    {
        if(id_to_column.find(id) == id_to_column.end()) {
            std::cerr << "You are trying to use unknown ID " << id << " in CSV_GENERATOR::AddData()" << std::endl;
            exit(1);
        }
        columns[id_to_column[id]].data.push_back(data);
    }

    void Generate(std::ofstream &ofs)
    {
        IsConsistent();
        const int ncolumns = static_cast<int>(columns.size());
        size_t rows=0;
        if(ncolumns > 0) {
            rows = columns[0].data.size();
        }

        for(int i=0; i<ncolumns; i++) {
            ofs << Linify(columns[i].header);
            if(i < ncolumns-1) {
                ofs << "\t";
            }
        }
        ofs << std::endl;

        for(size_t r=0; r<rows; r++) {
            for(int i=0; i<ncolumns; i++) {
                ofs << Linify(columns[i].data[r]);
                if(i < ncolumns-1) {
                    ofs << "\t";
                }
            }
            ofs << std::endl;
        }
    }

    void Normalize()
    {
        const int ncolumns = static_cast<int>(columns.size());
        size_t max_entries = 0;
        for(int i=0; i<ncolumns; i++) {
            max_entries = std::max(max_entries,columns[i].data.size());
        }
        for(int i=0; i<ncolumns; i++) {
            size_t this_size = columns[i].data.size();
            while(this_size++ < max_entries) {
                columns[i].data.push_back("");
            }
        }
    }

    void Generate(const std::string &filename)
    {
        IsConsistent();

        std::ofstream ofs;
        ofs.open(filename,std::ofstream::out);
        if(!ofs.is_open()) {
            std::cerr << "Couldn't open \"" << filename << "\" for output.";
            exit(1);
        }
        Generate(ofs);
        ofs.close();
    }
};

#endif
