#!/usr/bin/env node
"use strict";
const fs = require("node:fs");
const path = require("node:path");
const Ajv2020 = require("ajv/dist/2020");
const schemaPath = path.resolve(__dirname,"../schemas/k2040-settings-manifest.v1.schema.json");
const schema = JSON.parse(fs.readFileSync(schemaPath,"utf8"));
const ajv = new Ajv2020({allErrors:true,strict:true,strictRequired:false,validateFormats:false});
const check = ajv.compile(schema);

// This is a source/build-time gate, not a renderer or a values writer.
// JSON Schema checks structure; these rules check cross-field semantics.
function validateManifest(manifest) {
    const errors=[];
    if (!check(manifest)) {
        for (const e of check.errors||[]) {
            errors.push((e.instancePath || "/")+" "+e.message);
        }
        return errors;
    }
    const categoryIds=new Set();
    const fields=new Map();
    const actionIds=new Set();
    for (const category of manifest.categories) {
        if (categoryIds.has(category.id)) errors.push("duplicate category "+category.id);
        categoryIds.add(category.id);
        const groupIds=new Set();
        for (const group of category.groups) {
            if (groupIds.has(group.id))
                errors.push("duplicate group "+category.id+"/"+group.id);
            groupIds.add(group.id);
            for (const field of group.fields) {
                if (fields.has(field.id)) errors.push("duplicate setting "+field.id);
                fields.set(field.id,field);
                if (field.type==="action") {
                    if (actionIds.has(field.actionId))
                        errors.push("duplicate action "+field.actionId);
                    actionIds.add(field.actionId);
                    continue;
                }
                if (field.min!==undefined) {
                    if (!(field.max>field.min))
                        errors.push("invalid range for "+field.id);
                    if (!Number.isFinite(field.min)||!Number.isFinite(field.max)||
                        !Number.isFinite(field.step)||
                        !Number.isFinite((field.max-field.min)/field.step))
                        errors.push("non-finite numeric bound "+field.id);
                    if (field.type==="integer" &&
                        (![field.min,field.max,field.step].every(Number.isInteger)))
                        errors.push("integer field has fractional bound "+field.id);
                }
                if (field.type==="select") {
                    const seen=new Set();
                    const type=typeof field.options[0].value;
                    for (const opt of field.options) {
                        if (typeof opt.value!==type)
                            errors.push("mixed option types "+field.id);
                        const key=typeof opt.value+":"+opt.value;
                        if (seen.has(key)) errors.push("duplicate option "+field.id+"/"+key);
                        seen.add(key);
                    }
                }
            }
        }
    }
    const internals = new Set(manifest.internalKeys||[]);
    for (const key of internals) {
        if (fields.has(key)) errors.push("internal and visible setting collide "+key);
    }
    const graph=new Map();
    for (const [id,field] of fields) {
        const dependencies=field.visibleWhen?
            [...(field.visibleWhen.all||[]),...(field.visibleWhen.any||[])]:[];
        for (const dep of dependencies) {
            if (dep===id) errors.push("self dependency "+id);
            if (!fields.has(dep)) errors.push("missing dependency "+id+" -> "+dep);
            else if (fields.get(dep).type!=="toggle")
                errors.push("dependency must be a toggle "+id+" -> "+dep);
        }
        graph.set(id,dependencies);
    }
    const visitState=new Map();
    function visit(id) {
        if (visitState.get(id)===2) return;
        if (visitState.get(id)===1) {errors.push("visibility dependency cycle "+id);return;}
        visitState.set(id,1);
        for (const dep of graph.get(id)||[]) {
            if (graph.has(dep)) visit(dep);
        }
        visitState.set(id,2);
    }
    for (const id of graph.keys()) visit(id);
    return errors;
}
function cli(args) {
    if (!args.length) {
        process.stderr.write("Usage: validate-settings-manifest <manifest.json> [...]\n");
        return 2;
    }
    let failures=0;
    for (const filename of args) {
        try {
            const value=JSON.parse(fs.readFileSync(filename,"utf8"));
            const errors=validateManifest(value);
            if (errors.length) {
                ++failures;
                process.stderr.write("FAIL "+path.basename(filename)+": "+
                    errors.join("; ")+"\n");
            } else {
                process.stdout.write("PASS "+path.basename(filename)+
                    " ("+value.moduleId+", schema v"+value.manifestVersion+")\n");
            }
        } catch (e) {
            ++failures;
            process.stderr.write("FAIL "+path.basename(filename)+": "+
                e.message+"\n");
        }
    }
    return failures?1:0;
}
module.exports={schema,validateManifest,cli};
if (require.main===module) process.exitCode=cli(process.argv.slice(2));
