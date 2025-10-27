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

import com.google.common.flogger.FluentLogger
import java.util.concurrent.CancellationException
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import kotlinx.serialization.json.putJsonArray

/**
 * Represents a conversation with the LiteRT-LM model.
 *
 * Example usage:
 * ```kotlin
 * // Assuming 'engine' is an instance of LiteRtLm
 * val conversation = engine.createConversation()
 *
 * conversation.sendMessageAsync(
 *   Message.of("Hello world"),
 *   object : MessageCallback {
 *     override fun onMessage(message: Message) {
 *       // Handle the streaming response
 *       println("Response: ${message.contents[0] as Content.Text).text}")
 *     }
 *
 *     override fun onDone() {
 *       // Handle the end of the response
 *       println("Done")
 *     }
 *
 *     override fun onError(error: Throwable) {
 *       // Handle any errors
 *       println("Error: ${error.message}")
 *     }
 *   },
 * )
 *
 * // Close the conversation at the end to free the underlying native resource.
 * conversation.close()
 * ```
 *
 * This class is AutoCloseable, so you can use `use` block to ensure resources are released.
 *
 * @property handle The native handle to the conversation object.
 * @property toolManager The ToolManager instance to use for this conversation.
 */
class Conversation(private val handle: Long, val toolManager: ToolManager) : AutoCloseable {
  private val _isAlive = AtomicBoolean(true)

  /** Whether the conversation is alive and ready to be used, */
  val isAlive: Boolean
    get() = _isAlive.get()

  /**
   * Sends a message to the model and returns the response. This is a synchronous call.
   *
   * This method handles potential tool calls returned by the model. If a tool call is detected, the
   * corresponding tool is executed, and the result is sent back to the model. This process is
   * repeated until the model returns a final response without tool calls, up to
   * [RECURRING_TOOL_CALL_LIMIT] times.
   *
   * @param message The message to send to the model.
   * @return The model's response message.
   * @throws IllegalStateException if the conversation is not alive, if the native layer returns an
   *   invalid response, or if the tool call limit is exceeded.
   * @throws LiteRtLmJniException if an error occurs during the native call.
   */
  fun sendMessage(message: Message): Message {
    checkIsAlive()

    @Suppress("CheckReturnValue")
    var currentMessageJson = buildJsonObject {
      put("role", "user")
      put("content", message.toJson())
    }
    for (i in 0..<RECURRING_TOOL_CALL_LIMIT) {
      val responseJsonString = LiteRtLmJni.nativeSendMessage(handle, currentMessageJson.toString())
      val responseJsonObject = Json.decodeFromString<JsonObject>(responseJsonString)

      if ("tool_calls" in responseJsonObject) {
        currentMessageJson = handleToolCalls(responseJsonObject)
        // Loop back to send the tool response
      } else if ("content" in responseJsonObject) {
        return jsonToMessage(responseJsonObject)
      } else {
        throw IllegalStateException("Invalid response from native layer: $responseJsonString")
      }
    }
    throw IllegalStateException("Exceeded recurring tool call limit of $RECURRING_TOOL_CALL_LIMIT")
  }

  /**
   * Send message from the given contents in a async fashion.
   *
   * This method handles potential tool calls returned by the model. If a tool call is detected, the
   * corresponding tool is executed, and the result is sent back to the model. This process is
   * repeated until the model returns a final response without tool calls, up to
   * [RECURRING_TOOL_CALL_LIMIT] times.
   *
   * @param message The message to send to the model.
   * @param callback The callback to receive the streaming responses.
   * @throws IllegalStateException if the conversation has already been closed or the content is
   *   empty.
   */
  fun sendMessageAsync(message: Message, callback: MessageCallback) {
    checkIsAlive()

    val jniCallback = JniMessageCallbackImpl(callback)
    @Suppress("CheckReturnValue")
    val messageJSONObject = buildJsonObject {
      put("role", "user")
      put("content", message.toJson())
    }
    LiteRtLmJni.nativeSendMessageAsync(handle, messageJSONObject.toString(), jniCallback)
  }

  @Suppress("CheckReturnValue")
  private fun createToolResponse(functionName: String, result: String): JsonObject {
    return buildJsonObject {
      put("type", "tool_response")
      put(
        "tool_response",
        buildJsonObject {
          put("name", functionName)
          put("value", result)
        },
      )
    }
  }

  @Suppress("CheckReturnValue")
  private fun handleToolCalls(toolCallsJsonObject: JsonObject): JsonObject {
    return buildJsonObject {
      put("role", "tool")
      putJsonArray("content") {
        val toolCallsJSONArray = toolCallsJsonObject["tool_calls"]!!.jsonArray
        for (toolCallJSONObject in toolCallsJSONArray) {
          val toolCall = toolCallJSONObject.jsonObject
          if (!("function" in toolCall)) {
            logger.atWarning().log("handleToolCalls: 'function' key not found in tool call")
            continue
          }

          val functionJSONObject = toolCall["function"]?.jsonObject
          if (functionJSONObject == null) {
            logger.atWarning().log("handleToolCalls: 'function' key is not a JsonObject")
            continue
          }

          val functionName = functionJSONObject["name"]?.jsonPrimitive?.content
          val arguments = functionJSONObject["arguments"]?.jsonObject

          if (functionName == null) {
            logger.atWarning().log("handleToolCalls: 'name' key not found in function call")
            add(createToolResponse("error", "Invalid tool call format: function name not found"))
            continue
          }

          if (arguments == null) {
            logger.atWarning().log("handleToolCalls: 'arguments' key not found in function call")
            add(
              createToolResponse(
                functionName,
                "Error: Invalid tool call format: arguments not found",
              )
            )
            continue
          }

          logger.atInfo().log("handleToolCalls: Calling tools %s", functionName)
          val result = toolManager.execute(functionName, arguments)
          add(createToolResponse(functionName, result))
        }
      }
    }
  }

  private inner class JniMessageCallbackImpl(private val callback: MessageCallback) :
    LiteRtLmJni.JniMessageCallback {

    /** The tool response to be returned back */
    private var pendingToolResponseJSONMessage: JsonObject? = null
    private var toolCallCount = 0

    override fun onMessage(messageJsonString: String) {
      val messageJsonObject = Json.decodeFromString<JsonObject>(messageJsonString)

      if ("tool_calls" in messageJsonObject) {
        if (toolCallCount >= RECURRING_TOOL_CALL_LIMIT) {
          callback.onError(
            IllegalStateException(
              "Exceeded recurring tool call limit of $RECURRING_TOOL_CALL_LIMIT"
            )
          )
          return
        }
        toolCallCount++
        pendingToolResponseJSONMessage = handleToolCalls(messageJsonObject)
      } else if ("content" in messageJsonObject) {
        callback.onMessage(jsonToMessage(messageJsonObject))
      }
    }

    override fun onDone() {
      logger.atFine().log("onDone")
      val localToolResponse = pendingToolResponseJSONMessage
      if (localToolResponse != null) {
        // If there is pending tool response message, send the message.
        logger.atFine().log("onDone: Sending tool response.")
        LiteRtLmJni.nativeSendMessageAsync(
          handle,
          localToolResponse.toString(),
          this@JniMessageCallbackImpl,
        )
        logger.atFine().log("onDone: Tool response sent.")
        pendingToolResponseJSONMessage = null // Clear after sending
      } else {
        // If no pending action, then call onDone to the original user callback.
        callback.onDone()
      }
    }

    override fun onError(statusCode: Int, message: String) {
      logger.atFine().log("onError: %d, %s", statusCode, message)

      if (statusCode == 1) { // StatusCode::kCancelled
        callback.onError(CancellationException(message))
      } else {
        callback.onError(LiteRtLmJniException("Status Code: $statusCode. Message: $message"))
      }
    }
  }

  /**
   * Cancels any ongoing inference process.
   *
   * If there is no ongoing inference process, it is a no-op.
   *
   * @throws IllegalStateException if the session is not alive.
   */
  // b/450903294 is a pending feature request to roll the internal state back.
  fun cancelProcess() {
    checkIsAlive()
    LiteRtLmJni.nativeConversationCancelProcess(handle)
  }

  /**
   * Gets the benchmark info for the conversation.
   *
   * @return The benchmark info.
   * @throws IllegalStateException if the conversation is not alive.
   * @throws LiteRtLmJniException if benchmark is not enabled in the engine config.
   */
  @ExperimentalApi
  fun getBenchmarkInfo(): BenchmarkInfo {
    checkIsAlive()
    return LiteRtLmJni.nativeConversationGetBenchmarkInfo(handle)
  }

  /**
   * Closes the conversation and releases the native conversation's resources.
   *
   * @throws IllegalStateException if the conversation has already been closed.
   */
  override fun close() {
    if (_isAlive.compareAndSet(true, false)) {
      LiteRtLmJni.nativeDeleteConversation(handle)
    } else {
      throw IllegalStateException("Conversation is closed already.")
    }
  }

  /** Throws [IllegalStateException] if the conversation is not alive. */
  private fun checkIsAlive() {
    check(isAlive) { "Conversation is not alive." }
  }

  companion object {
    /**
     * The maximum number of times the model can call tools in a single turn before an error is
     * thrown.
     */
    private const val RECURRING_TOOL_CALL_LIMIT = 25
    private val logger = FluentLogger.forEnclosingClass()

    private fun jsonToMessage(messageJsonObject: JsonObject): Message {
      val contentsJsonArray = messageJsonObject["content"]!!.jsonArray
      val contents = mutableListOf<Content>()

      for (contentJsonObject in contentsJsonArray) {
        val content = contentJsonObject.jsonObject
        val type = content["type"]!!.jsonPrimitive.content

        if (type == "text") {
          contents.add(Content.Text(content["text"]!!.jsonPrimitive.content))
        } else {
          logger.atWarning().log("jsonToMessage: Got unsupported content type: %s", type)
        }
      }
      return Message.of(contents)
    }
  }
}

/** A callback for receiving streaming message responses. */
interface MessageCallback {
  /**
   * Called when a new message chunk is available from the model.
   *
   * This method may be called multiple times for a single `sendMessageAsync` call as the model
   * streams its response.
   *
   * @param contents The message chunk.
   */
  fun onMessage(message: Message)

  /**
   * Called when all message chunks are sent for a given `sendMessageAsync` call.
   *
   * If the model response includes tool calls, this method is called *after* the tool calls have
   * been executed and their responses have been sent back to the model.
   */
  fun onDone()

  /**
   * Called when an error occurs during the response streaming process.
   *
   * @param throwable The error that occurred.
   */
  fun onError(throwable: Throwable)
}
