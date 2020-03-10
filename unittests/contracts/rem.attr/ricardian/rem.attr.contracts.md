<h1 class="contract">create</h1>

---
spec_version: "1.0.0"
title: Create Attribute
summary: 'Creates attribute with the given name, type and scope'
icon: http://127.0.0.1/ricardian_assets/rem.contracts/icons/account.png#3d55a2fc3a5c20b456f5657faf666bc25ffd06f4836c5e8256f741149b0b294f
---

Creates attribute {{attribute_name}} of {{type}} and access scope {{ptype}}.

<h1 class="contract">invalidate</h1>

---
spec_version: "1.0.0"
title: Invalidate Attribute
summary: 'Invalidates attribute making it inaccessible'
icon: http://127.0.0.1/ricardian_assets/rem.contracts/icons/account.png#3d55a2fc3a5c20b456f5657faf666bc25ffd06f4836c5e8256f741149b0b294f
---

Invalidates attribute {{attribute_name}} disallowing setting it and making ready to be removed. 


<h1 class="contract">remove</h1>

---
spec_version: "1.0.0"
title: Remove Attribute
summary: 'Removes attribute'
icon: http://127.0.0.1/ricardian_assets/rem.contracts/icons/account.png#3d55a2fc3a5c20b456f5657faf666bc25ffd06f4836c5e8256f741149b0b294f
---

Removes previously invalidated attribute {{attribute_name}}. 


<h1 class="contract">setattr</h1>

---
spec_version: "1.0.0"
title: Set Attribute
summary: 'Set attribute to the {{receiver}}'
icon: http://127.0.0.1/ricardian_assets/rem.contracts/icons/account.png#3d55a2fc3a5c20b456f5657faf666bc25ffd06f4836c5e8256f741149b0b294f
---

Issue attribute {{attribute_name}} by {{issuer}} to {{receiver}} with the given value {{value}}

<h1 class="contract">unsetattr</h1>

---
spec_version: "1.0.0"
title: Reset Attribute
summary: 'Reset attribute to the {{receiver}}'
icon: http://127.0.0.1/ricardian_assets/rem.contracts/icons/account.png#3d55a2fc3a5c20b456f5657faf666bc25ffd06f4836c5e8256f741149b0b294f
---

Reset attribute {{attribute_name}} issued by {{issuer}} to {{receiver}}.
