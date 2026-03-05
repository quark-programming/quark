const { readFileSync, writeFileSync, readdirSync, lstatSync } = require("fs");

function path_to_entries(path) {
    if(lstatSync(path).isDirectory()) {
        return readdirSync(path).reduce((acc, file) =>
            acc.concat(path_to_entries(`${path}/${file}`)), [[path, null]]);
    }
    return [[path, readFileSync(path, 'utf8')]];
}

writeFileSync("build/lib-mirror.json", JSON.stringify(Object.fromEntries(path_to_entries("lib"))));