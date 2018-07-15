public Table loadCSV(String fname) {
    Table table = loadTable(fname + ".csv", "header");
    return table;
}