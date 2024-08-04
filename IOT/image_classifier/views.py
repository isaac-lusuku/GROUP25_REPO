# import os
# from rest_framework.response import Response
# from rest_framework import status
# from rest_framework.views import APIView
# from django.core.files.base import ContentFile
# from rest_framework.parsers import MultiPartParser, FormParser, JSONParser, FileUploadParser
# import cv2
# import numpy as np
# import tensorflow as tf
# from tensorflow.keras.preprocessing.image import img_to_array
# from tensorflow.keras.applications.resnet50 import ResNet50, preprocess_input, decode_predictions

# class ImageProcessor(APIView):
#     parser_classes = (FileUploadParser, MultiPartParser, FormParser, JSONParser)

#     def post(self, request):
#         try:
#             if 'file' not in request.FILES:
#                 return Response({"error": "No file uploaded"}, status=status.HTTP_400_BAD_REQUEST)

#             file = request.FILES['file']
#             image_data = file.read()
#             image = ContentFile(image_data, name=file.name)

#             # Process the image with OpenCV
#             np_img = np.frombuffer(image_data, np.uint8)
#             img = cv2.imdecode(np_img, cv2.IMREAD_COLOR)

#             if img is None:
#                 return Response({"error": "Image processing failed"}, status=status.HTTP_400_BAD_REQUEST)
            
#             # Display the image using OpenCV (optional, can be removed in production)
#             # cv2.imshow("Processed Image", img)
#             # cv2.waitKey(0)
#             # cv2.destroyAllWindows()

#             # Path to the model file
#             model_path = r"C:\Users\Administrator\Desktop\ML\Waste segmentation\Waste-classifier-retrained-specific-class.h5"

#             if not os.path.exists(model_path):
#                 return Response({"error": f"Model file not found at {model_path}"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

#             # Load the model
#             try:
#                 waste_model = tf.keras.models.load_model(model_path)
#             except Exception as e:
#                 return Response({"error": f"Error loading model: {str(e)}"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

#             class_labels = ["biodegradable", "non-biodegradable"]

#             # Preprocessing the image
#             img_8bit = cv2.convertScaleAbs(img)
#             # img_rgb = cv2.cvtColor(img_8bit, cv2.COLOR_BGR2RGB)  # Convert BGR to RGB
#             img_rgb = cv2.resize(img_8bit, (224, 224))  # Resize to 224x224
#             img_array = img_to_array(img_rgb)
#             img_array = np.expand_dims(img_array, axis=0)
#             img_array_processed = preprocess_input(img_array)

#             # Load ResNet50 model pre-trained on ImageNet
#             model = ResNet50(weights='imagenet')

#             # Use the model to predict the class of the image
#             predictions = model.predict(img_array_processed)

#             # Decode the predictions
#             decoded_predictions = decode_predictions(predictions, top=3)[0]

#             # Print the top-3 predictions
#             for i, (imagenet_id, label, score) in enumerate(decoded_predictions):
#                 print(f"{i+1}. {label}: {score:.2f}")

#             # Predicting the class
#             try:
#                 prediction = waste_model.predict(img_array_processed)
#                 predicted_class = np.argmax(prediction, axis=1)[0]
#                 predicted_class_name = class_labels[predicted_class]

#             except Exception as e:
#                 return Response({"error": f"Error during prediction: {str(e)}"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

#             print(f"The predicted class is {predicted_class_name}")
#             return Response("biodegradable", status=status.HTTP_200_OK)

#         except Exception as e:
#             # Catch all other exceptions and log the error
#             return Response({"error": f"An error occurred: {str(e)}"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)





import random
import matplotlib.pyplot as plt
import matplotlib.image as imgy
import os
import cv2
import numpy as np
import tensorflow as tf
from rest_framework.response import Response
from rest_framework import status
from rest_framework.views import APIView
from rest_framework.parsers import MultiPartParser, FormParser, JSONParser, FileUploadParser
from django.core.files.base import ContentFile
from django.conf import settings
from tensorflow.keras.preprocessing.image import img_to_array, load_img
from tensorflow.keras.applications.resnet50 import preprocess_input, decode_predictions

# Ensure the MEDIA_ROOT is set in your Django settings
MEDIA_ROOT = settings.MEDIA_ROOT if hasattr(settings, 'MEDIA_ROOT') else 'media/'

# Load the waste model once during the startup
# Path to the model file
model_path = r"C:\Users\Administrator\Desktop\ML\Waste segmentation\Waste-classifier-retrained-specific-class.h5"

if not os.path.exists(model_path):
    print(f"Model file not found at {model_path}")

# Load the model
try:
    waste_model = tf.keras.models.load_model(model_path)
except Exception as e:
    print(f"Error loading model: {str(e)}")
class_labels = ["biodegradable", "non-biodegradable"]

def predict_image_class(image_path):
    try:
        # Load and preprocess the image
        img = load_img(image_path, target_size=(224, 224))
        img_array = img_to_array(img)
        img_array = np.expand_dims(img_array, axis=0)
        img_array = preprocess_input(img_array)

        # displaying the image 
        # image = imgy.imread(image_path)
        # plt.imshow(image)

        # Predict the class
        prediction = waste_model.predict(img_array)
        predicted_class = np.argmax(prediction, axis=1)[0]
        predicted_class_name = class_labels[predicted_class]

        return predicted_class_name

    except Exception as e:
        print(f"Error during prediction: {str(e)}")
        return None

class ImageProcessor(APIView):
    parser_classes = (FileUploadParser, MultiPartParser, FormParser, JSONParser)

    def post(self, request):
        try:
            if 'file' not in request.FILES:
                return Response({"error": "No file uploaded"}, status=status.HTTP_400_BAD_REQUEST)

            file = request.FILES['file']
            temp_image_path = os.path.join(MEDIA_ROOT, file.name)
            
            # Save the uploaded file to the media directory
            with open(temp_image_path, 'wb') as temp_image_file:
                for chunk in file.chunks():
                    temp_image_file.write(chunk)

            # Perform the prediction
            predicted_class_name = predict_image_class(temp_image_path)

            # Delete the temporary image file
            os.remove(temp_image_path)

            if predicted_class_name:
                print(f"the predicted class is {predicted_class_name}")
                return Response(predicted_class_name, status=status.HTTP_200_OK)
            else:
                return Response({"error": "Prediction failed"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

        except Exception as e:
            # Catch all other exceptions and log the error
            return Response({"error": f"An error occurred: {str(e)}"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

# testing_path_O = r"C:\Users\Administrator\Desktop\ML\Waste segmentation\waste_resources\PROJECT_TEST"
# images_O = [f for f in os.listdir(testing_path_O) if os.path.isfile(os.path.join(testing_path_O, f))]
# selected_images = random.sample(images_O, 10)
# plt.figure(figsize=(10, 10))
# for img_file in selected_images:
#     img_path = os.path.join(testing_path_O, img_file)
#     img = load_img(img_path, target_size=(224, 224))
#     img_array = img_to_array(img)
#     img_array = np.expand_dims(img_array, axis=0)
#     clipped = img_array / 255.
#     predicted_class = predict_image_class(img_path)
#     plt.subplot(2, 5, selected_images.index(img_file)+1)
#     plt.imshow(clipped[0])
#     plt.title(predicted_class)
#     plt.axis('off')
# plt.show()