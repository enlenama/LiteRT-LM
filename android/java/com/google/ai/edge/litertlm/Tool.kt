/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ai.edge.litertlm

import kotlin.reflect.full.functions
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonArray
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.put
import kotlinx.serialization.json.putJsonArray
import kotlinx.serialization.json.putJsonObject

/**
 * Example of how to define tools:
 * - Use `@Tool` to define a method as a tool.
 * - Use `@ToolParam` to add information to the param of a tool.
 * - The allowed parameter types are: String, Int, Boolean, Float, Double, and List of them..
 * - The return type could be anything and will to converted with toString() back to the model.
 * - Use the Kotlin nullable type (e.g., String?) to indicate that a parameter is optional.
 *
 * ```kotlin
 * class MyToolSet {
 *   @Tool(description = "Get the current weather")
 *   fun getCurrentWeather(
 *     @ToolParam(description = "The city and state, e.g. San Francisco, CA") location: String,
 *     @ToolParam(description = "The temperature unit to use") unit: String = "celsius",
 *   ): JSONObject {
 *     return JSONObject(mapOf("temperture" to 25, "unit" to "Celsius"))
 *   }
 * }
 * ```
 */

/**
 * Annotation to mark a function as a tool that can be used by the LiteRT-LM model.
 *
 * @property description A description of the tool.
 */
@Target(AnnotationTarget.FUNCTION) // This annotation can only be applied to functions
@Retention(AnnotationRetention.RUNTIME) // IMPORTANT: Makes the annotation available at runtime
annotation class Tool(val description: String)

/**
 * Annotation to provide a description for a tool parameter.
 *
 * @property description A description of the tool parameter.
 */
@Target(AnnotationTarget.VALUE_PARAMETER) // This annotation can only be applied to functions
@Retention(AnnotationRetention.RUNTIME) // IMPORTANT: Makes the annotation available at runtime
annotation class ToolParam(val description: String)

/**
 * Represents a single tool, wrapping an instance and a specific Kotlin function.
 *
 * @property instance The instance of the class containing the tool function.
 * @property kFunction The Kotlin function to be executed as a tool.
 */
internal class Tooling(val instance: Any, val kFunction: kotlin.reflect.KFunction<*>) {

  companion object {
    private val javaTypeToJsonTypeString =
      mapOf(
        String::class to "string",
        Int::class to "integer",
        Boolean::class to "boolean",
        Float::class to "number",
        Double::class to "number",
        List::class to "array",
      )
  }

  /**
   * Executes the tool function with the given parameters.
   *
   * @param params A JsonObject containing the parameter names and their values.
   * @return The result of the tool function execution as a Any?.
   * @throws IllegalArgumentException if any required parameters are missing.
   */
  fun execute(params: JsonObject): Any? {
    val args =
      kFunction.parameters
        .associateWith { param ->
          when {
            param.index == 0 -> instance // First parameter is the instance
            param.name != null && param.name in params -> {
              val value = params[param.name!!]!!
              convertJsonValue(value, param.type)
            }
            param.isOptional -> null // Use default value
            else -> throw IllegalArgumentException("Missing parameter: ${param.name}")
          }
        }
        .filterValues { it != null }

    return kFunction.callBy(args)
  }

  /**
   * Converts a JSON value to the expected Kotlin type.
   *
   * @param value The JSON value to convert.
   * @param type The target Kotlin type.
   * @return The converted value.
   * @throws IllegalArgumentException if the value cannot be converted to the target type.
   */
  private fun convertJsonValue(value: JsonElement, type: kotlin.reflect.KType): Any {
    val classifier = type.classifier
    return when {
      classifier == List::class && value is JsonArray -> {
        val listTypeArgument = type.arguments.firstOrNull()?.type
        value.map { element -> convertJsonValue(element, listTypeArgument!!) }
      }
      classifier == Int::class && value is JsonPrimitive -> value.content.toInt()
      classifier == Float::class && value is JsonPrimitive -> value.content.toFloat()
      classifier == Double::class && value is JsonPrimitive -> value.content.toDouble()
      classifier == String::class && value is JsonPrimitive -> value.content
      classifier == Boolean::class && value is JsonPrimitive -> value.content.toBoolean()
      // Add more conversions if needed
      else -> throw IllegalArgumentException("Unsupported type conversion for $value to $type")
    }
  }

  /**
   * Generates a JSON schema for the given Kotlin type.
   *
   * @param type The Kotlin type to generate the schema for.
   * @return A JsonObject representing the JSON schema.
   * @throws IllegalArgumentException if the type is not supported.
   */
  private fun getTypeJsonSchema(type: kotlin.reflect.KType): JsonObject {
    val classifier = type.classifier
    val jsonType = javaTypeToJsonTypeString[classifier]

    if (jsonType == null) {
      throw IllegalArgumentException(
        "Unsupported type: ${classifier.toString()}. " +
          "Allowed types are: ${javaTypeToJsonTypeString.keys.joinToString { it.simpleName ?: "" }}"
      )
    }

    @Suppress("CheckReturnValue")
    return buildJsonObject {
      put("type", jsonType)
      if (classifier == List::class) {
        val listTypeArgument = type.arguments.firstOrNull()?.type
        if (listTypeArgument == null) {
          throw IllegalArgumentException("List type argument is missing.")
        }
        put("items", getTypeJsonSchema(listTypeArgument))
      }
    }
  }

  /**
   * Gets the tool description in Open API format.
   *
   * @return The tool description.
   */
  fun getToolDescription(): JsonObject {
    val toolAnnotation =
      kFunction.annotations.find { it is Tool } as? Tool ?: return buildJsonObject {}

    val parameters = kFunction.parameters.drop(1) // Drop the instance parameter

    @Suppress("CheckReturnValue")
    return buildJsonObject {
      put("name", kFunction.name)
      put("description", toolAnnotation.description)
      putJsonObject("parameters") {
        put("type", "object")
        putJsonObject("properties") {
          for (param in parameters) {
            val paramAnnotation = param.annotations.find { it is ToolParam } as? ToolParam
            putJsonObject(param.name!!) {
              for ((key, value) in getTypeJsonSchema(param.type)) {
                put(key, value)
              }
              // add "description" if provided
              paramAnnotation?.description?.let { put("description", it) }
              put("nullable", param.type.isMarkedNullable)
            }
          }
        }
        putJsonArray("required") {
          for (it in parameters.filter { !it.isOptional }) {
            add(JsonPrimitive(it.name))
          }
        }
      }
    }
  }
}

/**
 * Manages a collection of tool sets and provides methods to execute tools and get their
 * specifications.
 *
 * @property toolSets A list of objects, where each object contains methods annotated with @Tool.
 */
class ToolManager(val toolSets: List<Any>) {
  private val tools: Map<String, Tooling> =
    toolSets
      .flatMap { toolSet ->
        val toolClass = toolSet.javaClass.kotlin
        toolClass.functions
          .filter { function -> function.annotations.any { annotation -> annotation is Tool } }
          .map { function -> function.name to Tooling(toolSet, function) }
      }
      .toMap()

  /**
   * Executes a tool function by its name with the given parameters.
   *
   * @param functionName The name of the tool function to execute.
   * @param params A JsonObject containing the parameter names and their values.
   * @return The result of the tool function execution as a string.
   * @throws IllegalArgumentException if the tool function is not found.
   */
  fun execute(functionName: String, params: JsonObject): String {
    try {
      val tool =
        tools[functionName] ?: throw IllegalArgumentException("Tool not found: ${functionName}")
      val value = tool.execute(params)

      // specifical case when a Kotlin function return nothing.
      if (value is kotlin.Unit) {
        return ""
      } else {
        return value.toString()
      }
    } catch (e: Exception) {
      return "Error occured. ${e.toString()}"
    }
  }

  /**
   * Gets the tools description for all registered tools in Open API format.
   *
   * @return A json array of OpenAPI tool description JSON as string.
   */
  fun getToolsDescription(): JsonArray {
    return buildJsonArray {
      for (tool in tools.values) {
        @Suppress("CheckReturnValue") add(tool.getToolDescription())
      }
    }
  }
}
