"""Read only quest40419 definitions from the pinned local original TDB735 archive."""
import re
import sys
from pathlib import Path

columns, inside, found = [], False, {}
for line in Path(sys.argv[1]).open(encoding='utf-8'):
    if line.startswith('CREATE TABLE `quest_objectives`'):
        inside = True
        continue
    if inside:
        if line.startswith(')'):
            inside = False
        else:
            match = re.match(r'  `([^`]+)`', line)
            if match:
                columns.append(match[1])
    if not line.startswith('INSERT INTO `quest_objectives`'):
        continue
    for record in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        row = dict(zip(columns, re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+", record[0][1:-1])))
        if row['QuestID'] == '40419':
            found[int(row['ID'])] = tuple(int(row[field]) for field in ('Order', 'StorageIndex', 'ObjectID', 'Flags', 'VerifiedBuild'))
expected = {281978: (0, 4, 103481, 0, 25549), 281310: (1, 1, 102129, 2, 25549),
            281311: (2, 2, 102131, 2, 25549), 281980: (3, 5, 103490, 6, 25549),
            281574: (4, 3, 102548, 2, 25549)}
assert found == expected, found
for identity, values in sorted(found.items(), key=lambda pair: pair[1][0]):
    print(identity, *values, sep='\t')
