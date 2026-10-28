from pathlib import Path
import json,re
source=Path('browser/chromium/file-recognition.js').read_text(encoding='utf-8')
match=re.search(r' const types=(\{.*?\});',source)
types=json.loads(match.group(1))
lines=['#pragma once','#include <algorithm>','#include <iterator>','#include <string_view>','#include <utility>','// Generated from the bundled mime-db 1.54.0 data in browser/chromium/file-recognition.js.','// MIT license: third_party/mime-db-LICENSE.txt. Regenerate with tools/generate-native-mime.py.','namespace udm {','inline std::string_view mimeFilenameExtension(std::string_view type){',' static constexpr std::pair<std::string_view,std::string_view> entries[]={']
for key,values in sorted(types.items()):
 if values: lines.append('  {'+json.dumps(key)+','+json.dumps(values[0])+'},')
lines += [' };',' const auto found=std::lower_bound(std::begin(entries),std::end(entries),type,[](const auto& item,std::string_view value){return item.first<value;});',' return found!=std::end(entries)&&found->first==type?found->second:std::string_view{};','}','}']
Path('native/MimeFilenameData.hpp').write_text('\n'.join(lines)+'\n',encoding='utf-8')
Path('native/third_party/mime-db-LICENSE.txt').write_bytes(Path('browser/chromium/mime-db-LICENSE.txt').read_bytes())
print('Generated',len(types),'MIME mappings')
